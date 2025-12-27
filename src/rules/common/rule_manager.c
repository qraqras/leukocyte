#include <stdlib.h>
#include <string.h>
#include <fnmatch.h>
#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include <pthread.h>

#include "rules/common/rule_manager.h"
#include "configs/common/config.h"
#include "rules/common/rule.h"
#include "common/rule_registry.h"
#include "prism/diagnostic.h"

/* Array of rules by node type */
rule_t **rules_by_type[PM_NODE_TYPE_COUNT];
size_t rules_count_by_type[PM_NODE_TYPE_COUNT];

/* Simple accumulators for handler timing */
static uint64_t g_handler_time_ns = 0;
static size_t g_handler_calls = 0;

static inline uint64_t timespec_diff_ns(const struct timespec *a, const struct timespec *b)
{
    return (uint64_t)(b->tv_sec - a->tv_sec) * 1000000000ull + (uint64_t)(b->tv_nsec - a->tv_nsec);
}

/**
 * @brief Push an ancestor node onto the context's ancestor stack.
 * @param ctx Pointer to the rule_context_t structure
 * @param node Pointer to the pm_node_t ancestor node to push
 * @return true on success, false on failure
 */
static bool rule_context_push_ancestor(rule_context_t *ctx, pm_node_t *node)
{
    if (!ctx)
        return false;
    if (ctx->ancestors_count + 1 > ctx->ancestors_cap)
    {
        size_t newcap = ctx->ancestors_cap == 0 ? 8 : ctx->ancestors_cap * 2;
        pm_node_t **n = (pm_node_t **)realloc(ctx->ancestors, newcap * sizeof(pm_node_t *));
        if (!n)
            return false;
        ctx->ancestors = n;
        ctx->ancestors_cap = newcap;
    }
    ctx->ancestors[ctx->ancestors_count++] = node;
    return true;
}

/**
 * @brief Pop an ancestor node from the context's ancestor stack.
 * @param ctx Pointer to the rule_context_t structure
 */
static void rule_context_pop_ancestor(rule_context_t *ctx)
{
    if (!ctx || ctx->ancestors_count == 0)
        return;
    ctx->ancestors_count -= 1;
}

/* Forward declaration for visitor callback used by child traversal */
bool node_visitor(const pm_node_t *node, void *data);

/**
 * @brief Implementation of node visitor applying rules.
 */
static inline __attribute__((always_inline)) bool node_visitor_impl(const pm_node_t *node, rule_context_t *base)
{
    pm_node_type_t type = node->type;

    /* Copy base context for this callback; direct parent is last ancestor if present */
    rule_context_t ctx = *base;
    ctx.parent = (ctx.ancestors_count > 0) ? ctx.ancestors[ctx.ancestors_count - 1] : NULL;

    /* Optional debug logging; enable by defining RULE_MANAGER_DEBUG */
#ifdef RULE_MANAGER_DEBUG
    if (!base->diagnostics)
    {
        fprintf(stderr, "[rule_manager] warning: diagnostics pointer is NULL for node type %d\n", (int)type);
    }
    else
    {
        fprintf(stderr, "[rule_manager] diagnostics=%p size=%zu head=%p tail=%p for node type %d\n",
                (void *)base->diagnostics,
                base->diagnostics->size,
                (void *)base->diagnostics->head,
                (void *)base->diagnostics->tail,
                (int)type);
    }
#endif

    /* Prefer per-file rules if provided in base context, otherwise fall back to global rules */
    if (base->rules)
    {
        const rules_by_type_t *rb = base->rules;
        if (rb->rules_count_by_type[type] > 0)
        {
            for (size_t i = 0; i < rb->rules_count_by_type[type]; i++)
            {
                rule_t *rule = rb->rules_by_type[type][i];
                struct timespec t0, t1;
                clock_gettime(CLOCK_MONOTONIC, &t0);
                rule->handlers[type]((pm_node_t *)node, &ctx);
                clock_gettime(CLOCK_MONOTONIC, &t1);
                g_handler_time_ns += timespec_diff_ns(&t0, &t1);
                g_handler_calls += 1;
            }
        }
    }
    else
    {
        if (rules_count_by_type[type] > 0)
        {
            for (size_t i = 0; i < rules_count_by_type[type]; i++)
            {
                rule_t *rule = rules_by_type[type][i];
                struct timespec t0, t1;
                clock_gettime(CLOCK_MONOTONIC, &t0);
                rule->handlers[type]((pm_node_t *)node, &ctx);
                clock_gettime(CLOCK_MONOTONIC, &t1);
                g_handler_time_ns += timespec_diff_ns(&t0, &t1);
                g_handler_calls += 1;
            }
        }
    }

    /* Traverse children with ancestor stack: push current node, visit children, then pop */
    pm_node_t *prev_parent = base->parent;
    if (rule_context_push_ancestor(base, (pm_node_t *)node))
    {
        base->parent = (pm_node_t *)node;
        pm_visit_child_nodes(node, node_visitor, base);
        base->parent = prev_parent;
        rule_context_pop_ancestor(base);
    }
    else
    {
        /* Allocation failed; fallback to visiting children without ancestor tracking */
        pm_visit_child_nodes(node, node_visitor, base);
    }

    /* We handled children ourselves; avoid letting pm_visit_node recurse again */
    return false;
}

/**
 * @brief Visitor callback for AST nodes applying rules.
 */
bool node_visitor(const pm_node_t *node, void *data)
{
    return node_visitor_impl(node, (rule_context_t *)data);
}

/**
 * @brief Visit a node and apply relevant rules.
 * @param node Pointer to the AST node
 * @param parser Pointer to the parser
 * @param diagnostics Pointer to the diagnostics list
 * @param cfg Pointer to the configuration
 * @return true on success, false on failure
 */
bool visit_node(pm_node_t *node, pm_parser_t *parser, pm_list_t *diagnostics, leuko_config_t *cfg)
{
    pm_list_t local_list = {0};
    pm_list_t *diag = diagnostics ? diagnostics : &local_list;

    /* Build base context */
    leuko_processed_source_t ps = {0};
    rule_context_t ctx = {.cfg = cfg, .rules = NULL, .ps = NULL, .parser = parser, .diagnostics = diag, .parent = NULL, .ancestors = NULL, .ancestors_count = 0, .ancestors_cap = 0};

    /* Initialize processed_source once per visit and attach to context */
    leuko_processed_source_init_from_parser(&ps, parser);
    ctx.ps = &ps;

    pm_visit_node(node, node_visitor, &ctx);

    /* Free ancestor stack and any allocations in processed_source prepared for this visit */
    if (ctx.ancestors)
    {
        free(ctx.ancestors);
        ctx.ancestors = NULL;
        ctx.ancestors_count = 0;
        ctx.ancestors_cap = 0;
    }
    leuko_processed_source_free(&ps);

    if (!diagnostics)
    {
        /* Use pm_diagnostic_list_free to ensure owned message strings are freed */
        pm_diagnostic_list_free(&local_list);
    }
    return true;
}

/* Visit with an explicit per-file rules set */
bool visit_node_with_rules(pm_node_t *node, pm_parser_t *parser, pm_list_t *diagnostics, leuko_config_t *cfg, const rules_by_type_t *rules)
{
    pm_list_t local_list = {0};
    pm_list_t *diag = diagnostics ? diagnostics : &local_list;

    /* Build base context with per-file rules */
    leuko_processed_source_t ps = {0};
    rule_context_t ctx = {.cfg = cfg, .rules = rules, .ps = NULL, .parser = parser, .diagnostics = diag, .parent = NULL, .ancestors = NULL, .ancestors_count = 0, .ancestors_cap = 0};

    /* Initialize processed_source once per visit and attach to context */
    leuko_processed_source_init_from_parser(&ps, parser);
    ctx.ps = &ps;

    pm_visit_node(node, node_visitor, &ctx);

    /* Free ancestor stack and any allocations in processed_source prepared for this visit */
    if (ctx.ancestors)
    {
        free(ctx.ancestors);
        ctx.ancestors = NULL;
        ctx.ancestors_count = 0;
        ctx.ancestors_cap = 0;
    }
    leuko_processed_source_free(&ps);

    if (!diagnostics)
    {
        /* Use pm_diagnostic_list_free to ensure owned message strings are freed */
        pm_diagnostic_list_free(&local_list);
    }
    return true;
}

/* Helper: add a rule to a rules_by_type_t for a given node index. Uses doubling allocation. */
static bool add_rule_to_rb(rules_by_type_t *rb, size_t node_idx, rule_t *r)
{
    /* Maintain a separate capacity array in implicit fashion: use realloc with doubling */
    size_t cur = rb->rules_count_by_type[node_idx];
    size_t new_cap = cur == 0 ? 4 : cur * 2;
    rule_t **newarr = realloc(rb->rules_by_type[node_idx], new_cap * sizeof(rule_t *));
    if (!newarr)
    {
        return false;
    }
    rb->rules_by_type[node_idx] = newarr;
    rb->rules_by_type[node_idx][cur] = r;
    rb->rules_count_by_type[node_idx] = cur + 1;
    return true;
}

/* Helper: match file against patterns (fnmatch). */
static bool file_matches_patterns(const char *file_path, char **patterns, size_t count)
{
    if (!patterns || count == 0)
    {
        return false;
    }
    for (size_t i = 0; i < count; i++)
    {
        if (!patterns[i])
        {
            continue;
        }
        if (fnmatch(patterns[i], file_path, 0) == 0)
        {
            return true;
        }
    }
    return false;
}

/* Build rules_by_type for a specific file based on configuration. */
bool build_rules_by_type_for_file(const leuko_config_t *cfg, const char *file_path, rules_by_type_t *out)
{
    if (!cfg || !file_path || !out)
    {
        return false;
    }

    memset(out, 0, sizeof(*out));

    const leuko_registry_category_t *cats = leuko_get_rule_categories();
    size_t cats_n = leuko_get_rule_category_count();

    for (size_t ci = 0; ci < cats_n; ci++)
    {
        const leuko_registry_category_t *cat = &cats[ci];
        /* Find runtime category config if present */
        const leuko_config_category_base_t *cc = leuko_config_get_category_config((leuko_config_t *)cfg, cat->name);
        if (!cc)
            continue;
        for (size_t ei = 0; ei < cat->count; ei++)
        {
            const leuko_registry_rule_entry_t *ent = &cat->entries[ei];
            rule_t *r = (rule_t *)ent->rule;

            /* Find rule config inside category */
            leuko_config_rule_base_t *rcfg = leuko_config_get_rule((leuko_config_t *)cfg, cat->name, ent->name);

            /* If no config or disabled, skip */
            if (!rcfg || !rcfg->enabled)
                continue;

            /* Include/Exclude semantics: if include list exists, file must match one; then exclude overrides */
            if (rcfg->include_count > 0)
            {
                if (!file_matches_patterns(file_path, rcfg->include, rcfg->include_count))
                    continue;
            }
            if (rcfg->exclude_count > 0)
            {
                if (file_matches_patterns(file_path, rcfg->exclude, rcfg->exclude_count))
                    continue;
            }

            /* Add rule for every node type it handles */
            for (size_t node = 0; node < PM_NODE_TYPE_COUNT; node++)
            {
                if (!r->handlers[node])
                    continue;
                if (!add_rule_to_rb(out, node, r))
                {
                    /* on allocation failure, free what we built and return false */
                    free_rules_by_type(out);
                    return false;
                }
            }
        }
    }

    return true;
}

/* Free rules_by_type arrays (caller-owned). */
void free_rules_by_type(rules_by_type_t *rb)
{
    if (!rb)
    {
        return;
    }
    for (size_t i = 0; i < PM_NODE_TYPE_COUNT; i++)
    {
        free(rb->rules_by_type[i]);
        rb->rules_by_type[i] = NULL;
        rb->rules_count_by_type[i] = 0;
    }
}

/* Simple cache of per-file rules_by_type keyed by file path and config pointer. */
typedef struct
{
    char *file_path;
    const leuko_config_t *cfg;
    rules_by_type_t rules;
} rules_cache_entry_t;

static rules_cache_entry_t *rules_cache = NULL;
static size_t rules_cache_count = 0;
static size_t rules_cache_cap = 0;

/* Mutex protecting the rules cache for thread-safe access */
static pthread_mutex_t rules_cache_lock = PTHREAD_MUTEX_INITIALIZER;

const rules_by_type_t *get_rules_by_type_for_file(const leuko_config_t *cfg, const char *file_path)
{
    if (!cfg || !file_path)
    {
        return NULL;
    }

    /* Fast-path: check under lock for an existing entry */
    pthread_mutex_lock(&rules_cache_lock);
    for (size_t i = 0; i < rules_cache_count; i++)
    {
        if (rules_cache[i].cfg == cfg && strcmp(rules_cache[i].file_path, file_path) == 0)
        {
            const rules_by_type_t *res = &rules_cache[i].rules;
            pthread_mutex_unlock(&rules_cache_lock);
            return res;
        }
    }
    pthread_mutex_unlock(&rules_cache_lock);

    /* Not found: build a temporary rules_by_type_t without holding the lock */
    rules_by_type_t tmp = {0};
    if (!build_rules_by_type_for_file(cfg, file_path, &tmp))
    {
        free_rules_by_type(&tmp);
        return NULL;
    }

    /* Re-acquire lock and check again (another thread may have inserted) */
    pthread_mutex_lock(&rules_cache_lock);
    for (size_t i = 0; i < rules_cache_count; i++)
    {
        if (rules_cache[i].cfg == cfg && strcmp(rules_cache[i].file_path, file_path) == 0)
        {
            /* Use existing entry and discard our temporary */
            const rules_by_type_t *res = &rules_cache[i].rules;
            pthread_mutex_unlock(&rules_cache_lock);
            free_rules_by_type(&tmp);
            return res;
        }
    }

    /* Insert our built entry */
    if (rules_cache_count == rules_cache_cap)
    {
        size_t newcap = rules_cache_cap == 0 ? 8 : rules_cache_cap * 2;
        rules_cache_entry_t *n = realloc(rules_cache, newcap * sizeof(rules_cache_entry_t));
        if (!n)
        {
            pthread_mutex_unlock(&rules_cache_lock);
            free_rules_by_type(&tmp);
            return NULL;
        }
        rules_cache = n;
        rules_cache_cap = newcap;
    }

    rules_cache_entry_t *entry = &rules_cache[rules_cache_count];
    entry->file_path = strdup(file_path);
    if (!entry->file_path)
    {
        pthread_mutex_unlock(&rules_cache_lock);
        free_rules_by_type(&tmp);
        return NULL;
    }
    entry->cfg = cfg;
    entry->rules = tmp; /* struct copy; tmp no longer needs explicit free */
    rules_cache_count++;
    const rules_by_type_t *res = &entry->rules;
    pthread_mutex_unlock(&rules_cache_lock);
    return res;
}

void rule_manager_clear_cache(void)
{
    pthread_mutex_lock(&rules_cache_lock);
    for (size_t i = 0; i < rules_cache_count; i++)
    {
        free(rules_cache[i].file_path);
        free_rules_by_type(&rules_cache[i].rules);
    }
    free(rules_cache);
    rules_cache = NULL;
    rules_cache_count = 0;
    rules_cache_cap = 0;
    pthread_mutex_unlock(&rules_cache_lock);
}

/**
 * @brief Initialize the rule manager by populating rules by node type.
 * @param void
 */
void init_rules(void)
{
    memset(rules_by_type, 0, sizeof(rules_by_type));
    memset(rules_count_by_type, 0, sizeof(rules_count_by_type));

    const leuko_registry_category_t *cats = leuko_get_rule_categories();
    size_t cats_n = leuko_get_rule_category_count();

    for (size_t ci = 0; ci < cats_n; ci++)
    {
        const leuko_registry_category_t *cat = &cats[ci];
        for (size_t ei = 0; ei < cat->count; ei++)
        {
            const leuko_registry_rule_entry_t *ent = &cat->entries[ei];
            rule_t *r = (rule_t *)ent->rule;
            for (size_t node = 0; node < PM_NODE_TYPE_COUNT; node++)
            {
                if (!r->handlers[node])
                {
                    continue;
                }

                size_t cur = rules_count_by_type[node];
                rule_t **newarr = realloc(rules_by_type[node], (cur + 1) * sizeof(rule_t *));
                if (!newarr)
                {
                    continue;
                }
                newarr[cur] = r;
                rules_by_type[node] = newarr;
                rules_count_by_type[node] = cur + 1;
            }
        }
    }
}

/* Timing helpers */
void rule_manager_reset_timing(void)
{
    g_handler_time_ns = 0;
    g_handler_calls = 0;
}

void rule_manager_get_timing(uint64_t *time_ns, size_t *calls)
{
    if (time_ns)
    {
        *time_ns = g_handler_time_ns;
    }
    if (calls)
    {
        *calls = g_handler_calls;
    }
}

#include <inttypes.h>

void rule_manager_dump_timings(void)
{
    fprintf(stderr, "[rule_manager] handler_time_ns=%" PRIu64 " ns\n", g_handler_time_ns);
    fprintf(stderr, "[rule_manager] handler_calls=%zu\n", g_handler_calls);
}
