/* Worker pool based pipeline for parallel file processing with deterministic ordering */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "worker/pipeline.h"
#include "parse.h"
#include "configs/discovery/discover.h"
#include "rules/rule_manager.h"
#include "allocator/prism_xallocator.h"
#include "prism/diagnostic.h"
#include "cli/formatter.h"

/**
 * @brief Task structure representing a file to process.
 */
typedef struct
{
    char *file;
    size_t idx;
} leuko_task_t;

/**
 * @brief Result structure representing the outcome of processing a file.
 */
typedef struct
{
    bool done;
    bool success;
    /* Serialized diagnostics copied out of thread-local pm_list_t (heap owned) */
    struct
    {
        pm_diagnostic_id_t diag_id;
        char *message;
        uint8_t level;
    } *diags;
    size_t diags_count;
} leuko_result_t;

/**
 * @brief Thread-safe task queue.
 */
typedef struct
{
    leuko_task_t *tasks;
    size_t head;
    size_t tail;
    size_t cap;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    bool closed;
} leuko_task_queue_t;

/**
 * @brief Initialize the task queue.
 * @param q Pointer to the task queue
 * @return true on success, false on failure
 */
static bool leuko_tq_init(leuko_task_queue_t *q)
{
    q->tasks = NULL;
    q->head = q->tail = 0;
    q->cap = 0;
    q->closed = false;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->cond, NULL);
    return true;
}

/**
 * @brief Destroy the task queue.
 * @param q Pointer to the task queue
 */
static void leuko_tq_destroy(leuko_task_queue_t *q)
{
    free(q->tasks);
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->cond);
}

/**
 * @brief Push a task onto the queue.
 * @param q Pointer to the task queue
 * @param file File path to process
 * @param idx Index of the task
 * @return true on success, false on failure
 */
static bool leuko_tq_push(leuko_task_queue_t *q, const char *file, size_t idx)
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
        leuko_task_t *n = realloc(q->tasks, newcap * sizeof(leuko_task_t));
        if (!n)
        {
            pthread_mutex_unlock(&q->lock);
            return false;
        }
        q->tasks = n;
        q->cap = newcap;
    }
    q->tasks[q->tail++ % q->cap] = (leuko_task_t){strdup(file), idx};
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->lock);
    return true;
}

/**
 * @brief Pop a task from the queue.
 * @param q Pointer to the task queue
 * @param out Pointer to store the popped task
 * @return true on success, false if the queue is closed and empty
 */
static bool leuko_tq_pop(leuko_task_queue_t *q, leuko_task_t *out)
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

/**
 * @brief Close the task queue.
 * @param q Pointer to the task queue
 */
static void leuko_tq_close(leuko_task_queue_t *q)
{
    pthread_mutex_lock(&q->lock);
    q->closed = true;
    pthread_cond_broadcast(&q->cond);
    pthread_mutex_unlock(&q->lock);
}

/**
 * @brief Worker context structure.
 */
typedef struct leuko_worker_ctx_s
{
    leuko_task_queue_t *q;
    leuko_result_t *results;
    leuko_config_t *cfg;
} leuko_worker_ctx_t;

/**
 * @brief Worker thread entrypoint.
 */
static void *leuko_worker_thread(void *v)
{
    leuko_worker_ctx_t *ctx = v;
    leuko_task_t task;
    while (leuko_tq_pop(ctx->q, &task))
    {
        leuko_result_t res = {0};
        pm_node_t *root = NULL;
        pm_parser_t parser = {0};
        uint8_t *source = NULL;

        bool parsed = leuko_parse_ruby_file(task.file, &root, &parser, &source);
        if (!parsed)
        {
            res.success = false;
            ctx->results[task.idx] = res;
            free(task.file);
            continue;
        }

        /* Build rules (cached) using per-file config discovery (read-only lookup for workers) */
        const leuko_config_t *file_cfg = NULL;
        if (leuko_config_get_cached_config_for_file_ro(task.file, &file_cfg) != 0)
        {
            /* unexpected error: treat as no cached config */
            file_cfg = NULL;
        }
        const rules_by_type_t *rules = get_rules_by_type_for_file(file_cfg ? file_cfg : ctx->cfg, task.file);

        /* Visit and collect diagnostics */
        pm_list_t local_diag = {0};
        visit_node_with_rules(root, &parser, &local_diag, ctx->cfg, rules);

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
bool leuko_run_pipeline(char **files, size_t files_count, leuko_config_t *cfg, size_t workers_count, int *any_failures, int formatter)
{
    if (!files || files_count == 0 || !cfg || workers_count == 0)
    {
        return false;
    }

    leuko_task_queue_t q;
    leuko_tq_init(&q);

    leuko_result_t *results = calloc(files_count, sizeof(leuko_result_t));
    if (!results)
    {
        leuko_tq_destroy(&q);
        return false;
    }

    /* Spawn workers */
    pthread_t *ths = calloc(workers_count, sizeof(pthread_t));
    leuko_worker_ctx_t ctx = {.q = &q, .results = results, .cfg = cfg};
    for (size_t i = 0; i < workers_count; i++)
    {
        pthread_create(&ths[i], NULL, leuko_worker_thread, &ctx);
    }

    /* Enqueue tasks preserving input order */
    for (size_t i = 0; i < files_count; i++)
    {
        leuko_tq_push(&q, files[i], i);
    }

    /* Close the queue and wait for workers */
    leuko_tq_close(&q);
    for (size_t i = 0; i < workers_count; i++)
    {
        pthread_join(ths[i], NULL);
    }

    /* Aggregate results and print diagnostics in input order deterministically */
    int failures = 0;
    for (size_t i = 0; i < files_count; i++)
    {
        leuko_result_t *r = &results[i];
        if (!r->success)
            failures++;
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

    if (any_failures)
        *any_failures = failures;

    free(ths);
    free(results);
    leuko_tq_destroy(&q);
    return true;
}
