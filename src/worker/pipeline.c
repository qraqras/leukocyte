/* Worker pool based pipeline for parallel file processing with deterministic ordering */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <time.h>

#include "worker/pipeline.h"
#include "parse.h"
#include "rules/rule_manager.h"
#include "allocator/prism_xallocator.h"
#include "prism/diagnostic.h"
#include "cli/formatter.h"

typedef struct
{
    char *file;
    size_t idx;
} task_t;

typedef struct
{
    bool done;
    bool success;
    double parse_ms;
    double build_ms;
    double visit_ms;
    uint64_t handler_ns;
    size_t handler_calls;
    /* Serialized diagnostics copied out of thread-local pm_list_t (heap owned) */
    struct
    {
        pm_diagnostic_id_t diag_id;
        char *message; /* strdup'd */
        uint8_t level;
    } *diags;
    size_t diags_count;
} result_t;

/* Simple FIFO queue */
typedef struct
{
    task_t *tasks;
    size_t head;
    size_t tail;
    size_t cap;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    bool closed;
} task_queue_t;

static bool tq_init(task_queue_t *q)
{
    q->tasks = NULL;
    q->head = q->tail = 0;
    q->cap = 0;
    q->closed = false;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->cond, NULL);
    return true;
}

static void tq_destroy(task_queue_t *q)
{
    free(q->tasks);
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->cond);
}

static bool tq_push(task_queue_t *q, const char *file, size_t idx)
{
    pthread_mutex_lock(&q->lock);
    if (q->closed)
    {
        pthread_mutex_unlock(&q->lock);
        return false;
    }
    size_t len = q->tail - q->head;
    if (len + 1 >= q->cap)
    {
        size_t newcap = q->cap == 0 ? 16 : q->cap * 2;
        task_t *n = realloc(q->tasks, newcap * sizeof(task_t));
        if (!n)
        {
            pthread_mutex_unlock(&q->lock);
            return false;
        }
        q->tasks = n;
        q->cap = newcap;
    }
    q->tasks[q->tail++ % q->cap] = (task_t){strdup(file), idx};
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->lock);
    return true;
}

static bool tq_pop(task_queue_t *q, task_t *out)
{
    pthread_mutex_lock(&q->lock);
    while (q->head == q->tail && !q->closed)
    {
        pthread_cond_wait(&q->cond, &q->lock);
    }
    if (q->head == q->tail && q->closed)
    {
        pthread_mutex_unlock(&q->lock);
        return false;
    }
    *out = q->tasks[q->head++ % q->cap];
    pthread_mutex_unlock(&q->lock);
    return true;
}

static void tq_close(task_queue_t *q)
{
    pthread_mutex_lock(&q->lock);
    q->closed = true;
    pthread_cond_broadcast(&q->cond);
    pthread_mutex_unlock(&q->lock);
}

typedef struct worker_ctx_s
{
    task_queue_t *q;
    result_t *results;
    config_t *cfg;
    bool timings;
} worker_ctx_t;

static inline double timespec_to_ms(const struct timespec *a, const struct timespec *b)
{
    double ms = (b->tv_sec - a->tv_sec) * 1000.0 + (b->tv_nsec - a->tv_nsec) / 1e6;
    return ms;
}

static void *worker_thread(void *v)
{
    worker_ctx_t *ctx = v;
    task_t task;
    while (tq_pop(ctx->q, &task))
    {
        result_t res = {0};
        pm_node_t *root = NULL;
        pm_parser_t parser = {0};
        uint8_t *source = NULL;

        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        bool parsed = leuko_parse_ruby_file(task.file, &root, &parser, &source);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        res.parse_ms = timespec_to_ms(&t0, &t1);
        if (!parsed)
        {
            res.success = false;
            ctx->results[task.idx] = res;
            free(task.file);
            continue;
        }

        /* Build rules (cached) */
        clock_gettime(CLOCK_MONOTONIC, &t0);
        const rules_by_type_t *rules = get_rules_by_type_for_file(ctx->cfg, task.file);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        res.build_ms = timespec_to_ms(&t0, &t1);

        /* Visit and measure handler time using thread-local counters; collect diagnostics */
        pm_list_t local_diag = {0};
        rule_manager_reset_timing();
        clock_gettime(CLOCK_MONOTONIC, &t0);
        visit_node_with_rules(root, &parser, &local_diag, ctx->cfg, rules);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        res.visit_ms = timespec_to_ms(&t0, &t1);
        rule_manager_get_timing(&res.handler_ns, &res.handler_calls);

        /* Serialize diagnostics into heap-owned array so they survive arena teardown */
        pm_diagnostic_t *d = (pm_diagnostic_t *)local_diag.head;
        size_t dc = 0;
        for (pm_diagnostic_t *iter = d; iter != NULL; iter = (pm_diagnostic_t *)iter->node.next)
            dc++;
        if (dc > 0)
        {
            res.diags = calloc(dc, sizeof(*res.diags));
            if (res.diags)
            {
                size_t idx = 0;
                for (pm_diagnostic_t *iter = d; iter != NULL; iter = (pm_diagnostic_t *)iter->node.next)
                {
                    const char *msg = iter->message ? iter->message : pm_diagnostic_id_human(iter->diag_id);
                    res.diags[idx].message = strdup(msg);
                    res.diags[idx].diag_id = iter->diag_id;
                    res.diags[idx].level = iter->level;
                    idx++;
                }
                res.diags_count = dc;
            }
        }
        /* Free diagnostics allocated by parser/handlers (may free owned messages allocated via xcalloc) */
        pm_diagnostic_list_free(&local_diag);

        res.success = true;

        /* Cleanup parser and arena for this thread */
        pm_node_destroy(&parser, root);
        pm_parser_free(&parser);
        leuko_x_allocator_end();
        free(source);

        ctx->results[task.idx] = res;
        free(task.file);
    }
    return NULL;
}

bool leuko_run_pipeline(char **files, size_t files_count, config_t *cfg, size_t workers, bool timings, int *any_failures, double *total_parse_ms, double *total_build_ms, double *total_visit_ms, uint64_t *total_handler_ns, int formatter)
{
    if (!files || files_count == 0 || !cfg || workers == 0)
        return false;

    task_queue_t q;
    tq_init(&q);

    result_t *results = calloc(files_count, sizeof(result_t));
    if (!results)
    {
        tq_destroy(&q);
        return false;
    }

    /* Spawn workers */
    pthread_t *ths = calloc(workers, sizeof(pthread_t));
    worker_ctx_t ctx = {.q = &q, .results = results, .cfg = cfg, .timings = timings};
    for (size_t i = 0; i < workers; i++)
    {
        pthread_create(&ths[i], NULL, worker_thread, &ctx);
    }

    /* Enqueue tasks preserving input order */
    for (size_t i = 0; i < files_count; i++)
    {
        tq_push(&q, files[i], i);
    }

    /* Close the queue and wait for workers */
    tq_close(&q);
    for (size_t i = 0; i < workers; i++)
    {
        pthread_join(ths[i], NULL);
    }

    /* Aggregate results and print in order deterministically */
    int failures = 0;
    double tp = 0.0, tb = 0.0, tv = 0.0;
    uint64_t th_ns = 0;
    for (size_t i = 0; i < files_count; i++)
    {
        result_t *r = &results[i];
        if (!r->success)
            failures++;
        tp += r->parse_ms;
        tb += r->build_ms;
        tv += r->visit_ms;
        th_ns += r->handler_ns;
        if (timings)
        {
            printf("TIMING file=%s parse_ms=%.3f visit_ms=%.3f handlers_ms=%.3f handler_calls=%zu\n", files[i], r->parse_ms, r->visit_ms, r->handler_ns / 1e6, r->handler_calls);
        }
        /* Print diagnostics deterministically in file order */
        for (size_t di = 0; di < r->diags_count; di++)
        {
            /* Use CLI formatter to print diagnostics */
            extern int leuko_cli_formatter; /* set by main when invoking pipeline */
            /* runtime formatter parameter takes precedence if non-zero */
            int fmt = formatter != 0 ? formatter : leuko_cli_formatter;
            cli_formatter_print_diagnostic((cli_formatter_t)fmt, files[i], r->diags[di].message ? r->diags[di].message : "", (int)r->diags[di].diag_id, r->diags[di].level);
            free(r->diags[di].message);
        }
        free(r->diags);
    }

    if (total_parse_ms)
        *total_parse_ms = tp;
    if (total_build_ms)
        *total_build_ms = tb;
    if (total_visit_ms)
        *total_visit_ms = tv;
    if (total_handler_ns)
        *total_handler_ns = th_ns;
    if (any_failures)
        *any_failures = failures;

    free(ths);
    free(results);
    tq_destroy(&q);
    return true;
}
