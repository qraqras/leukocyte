#include <stdlib.h>
#include <string.h>
#include <fnmatch.h>
#include <stdint.h>
#include <time.h>
#include <stdio.h>

#include "rules/rule_manager.h"
#include "configs/generated_config.h"

#include "rules/rule.h"
#include "rule_registry.h"
#include "prism/diagnostic.h"

/* Array of rules by node type */
rule_t **rules_by_type[PM_NODE_TYPE_COUNT];
size_t rules_count_by_type[PM_NODE_TYPE_COUNT];

/* Data passed to visitor */
typedef struct
{
    pm_parser_t *parser;
    pm_list_t *diagnostics;
    config_t *cfg;
    const rules_by_type_t *rules; /* per-file rules set (optional) */

    /* processed_source prepared once per visit */
    processed_source_t ps;
    bool has_ps;
} visit_data_t;

/**
 * @brief Visitor function called for each node during AST traversal.
 * @param node Pointer to the current AST node
 * @param data Pointer to visit_data_t structure
 * @return true to continue visiting, false to stop
 */
/* Simple accumulators for handler timing */
static uint64_t g_handler_time_ns = 0;
static size_t g_handler_calls = 0;

static inline uint64_t timespec_diff_ns(const struct timespec *a, const struct timespec *b)
{
    return (uint64_t)(b->tv_sec - a->tv_sec) * 1000000000ull + (uint64_t)(b->tv_nsec - a->tv_nsec);
}

bool node_visitor(const pm_node_t *node, void *data)
{
    visit_data_t *visit_data = (visit_data_t *)data;
    pm_node_type_t type = node->type;

    /* Build rule context for this callback */
    rule_context_t ctx = {
        .cfg = visit_data->cfg,
        .ps = visit_data->has_ps ? &visit_data->ps : NULL,
        .parser = visit_data->parser,
        .diagnostics = visit_data->diagnostics,
        .parent = NULL,
    };

    /* Optional debug logging; enable by defining RULE_MANAGER_DEBUG */
#ifdef RULE_MANAGER_DEBUG
    if (!visit_data->diagnostics)
    {
        fprintf(stderr, "[rule_manager] warning: diagnostics pointer is NULL for node type %d\n", (int)type);
    }
    else
    {
        fprintf(stderr, "[rule_manager] diagnostics=%p size=%zu head=%p tail=%p for node type %d\n",
                (void *)visit_data->diagnostics,
                visit_data->diagnostics->size,
                (void *)visit_data->diagnostics->head,
                (void *)visit_data->diagnostics->tail,
                (int)type);
    }
#endif

    /* Prefer per-file rules if provided, otherwise fall back to global rules */
    if (visit_data->rules)
    {
        const rules_by_type_t *rb = visit_data->rules;
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

    return true;
}

/**
 * @brief Visit a node and apply relevant rules.
 * @param node Pointer to the AST node
 * @param parser Pointer to the parser
 * @param diagnostics Pointer to the diagnostics list
 * @param cfg Pointer to the configuration
 * @return true on success, false on failure
 */
bool visit_node(pm_node_t *node, pm_parser_t *parser, pm_list_t *diagnostics, config_t *cfg)
{
    pm_list_t local_list = {0};
    pm_list_t *diag = diagnostics ? diagnostics : &local_list;
    visit_data_t data = {parser, diag, cfg, NULL, {0}, false};
    /* Initialize processed_source once per visit */
    processed_source_init_from_parser(&data.ps, parser);
    data.has_ps = true;
    pm_visit_node(node, node_visitor, &data);
    /* Free any allocations in processed_source prepared for this visit */
    if (data.has_ps)
    {
        processed_source_free(&data.ps);
    }
    if (!diagnostics)
    {
        /* Use pm_diagnostic_list_free to ensure owned message strings are freed */
        pm_diagnostic_list_free(&local_list);
    }
    return true;
}

/* Visit with an explicit per-file rules set */
bool visit_node_with_rules(pm_node_t *node, pm_parser_t *parser, pm_list_t *diagnostics, config_t *cfg, const rules_by_type_t *rules)
{
    pm_list_t local_list = {0};
    pm_list_t *diag = diagnostics ? diagnostics : &local_list;
    visit_data_t data = {parser, diag, cfg, rules, {0}, false};
    /* Initialize processed_source once per visit */
    processed_source_init_from_parser(&data.ps, parser);
    data.has_ps = true;
    pm_visit_node(node, node_visitor, &data);
    /* Free any allocations in processed_source prepared for this visit */
    if (data.has_ps)
    {
        processed_source_free(&data.ps);
    }
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
bool build_rules_by_type_for_file(const config_t *cfg, const char *file_path, rules_by_type_t *out)
{
    if (!cfg || !file_path || !out)
    {
        return false;
    }

    memset(out, 0, sizeof(*out));

    const rule_registry_entry_t *registry = get_rule_registry();
    size_t registry_count = get_rule_registry_count();

    for (size_t i = 0; i < registry_count; i++)
    {
        const rule_registry_entry_t *entry = &registry[i];
        rule_t *r = entry->rule;
        rule_config_t *rcfg = get_rule_config_by_index((config_t *)cfg, i);

        /* If no config or disabled, skip */
        if (!rcfg || !rcfg->enabled)
        {
            continue;
        }

        /* Include/Exclude semantics: if include list exists, file must match one; then exclude overrides */
        if (rcfg->include_count > 0)
        {
            if (!file_matches_patterns(file_path, rcfg->include, rcfg->include_count))
            {
                continue;
            }
        }
        if (rcfg->exclude_count > 0)
        {
            if (file_matches_patterns(file_path, rcfg->exclude, rcfg->exclude_count))
            {
                continue;
            }
        }

        /* Add rule for every node type it handles */
        for (size_t node = 0; node < PM_NODE_TYPE_COUNT; node++)
        {
            if (!r->handlers[node])
            {
                continue;
            }
            if (!add_rule_to_rb(out, node, r))
            {
                /* on allocation failure, free what we built and return false */
                free_rules_by_type(out);
                return false;
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
    const config_t *cfg;
    rules_by_type_t rules;
} rules_cache_entry_t;

static rules_cache_entry_t *rules_cache = NULL;
static size_t rules_cache_count = 0;
static size_t rules_cache_cap = 0;

const rules_by_type_t *get_rules_by_type_for_file(const config_t *cfg, const char *file_path)
{
    if (!cfg || !file_path)
    {
        return NULL;
    }

    for (size_t i = 0; i < rules_cache_count; i++)
    {
        if (rules_cache[i].cfg == cfg && strcmp(rules_cache[i].file_path, file_path) == 0)
        {
            return &rules_cache[i].rules;
        }
    }

    /* Build new entry */
    if (rules_cache_count == rules_cache_cap)
    {
        size_t newcap = rules_cache_cap == 0 ? 8 : rules_cache_cap * 2;
        rules_cache_entry_t *n = realloc(rules_cache, newcap * sizeof(rules_cache_entry_t));
        if (!n)
        {
            return NULL;
        }
        rules_cache = n;
        rules_cache_cap = newcap;
    }

    rules_cache_entry_t *entry = &rules_cache[rules_cache_count];
    entry->file_path = strdup(file_path);
    entry->cfg = cfg;
    if (!build_rules_by_type_for_file(cfg, file_path, &entry->rules))
    {
        free(entry->file_path);
        entry->file_path = NULL;
        return NULL;
    }
    rules_cache_count++;
    return &entry->rules;
}

void rule_manager_clear_cache(void)
{
    for (size_t i = 0; i < rules_cache_count; i++)
    {
        free(rules_cache[i].file_path);
        free_rules_by_type(&rules_cache[i].rules);
    }
    free(rules_cache);
    rules_cache = NULL;
    rules_cache_count = 0;
    rules_cache_cap = 0;
}

/**
 * @brief Initialize the rule manager by populating rules by node type.
 * @param void
 */
void init_rules(void)
{
    memset(rules_by_type, 0, sizeof(rules_by_type));
    memset(rules_count_by_type, 0, sizeof(rules_count_by_type));

    const rule_registry_entry_t *registry = get_rule_registry();
    size_t registry_count = get_rule_registry_count();

    for (size_t i = 0; i < registry_count; i++)
    {
        rule_t *r = registry[i].rule;
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
