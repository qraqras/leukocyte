#ifndef LEUKOCYTE_RULE_MANAGER_H
#define LEUKOCYTE_RULE_MANAGER_H

#include "rules/rule.h"

// Initialize rules
void init_rules();

/* Rules-by-type structure for per-file enabled rule sets */
typedef struct
{
	rule_t **rules_by_type[PM_NODE_TYPE_COUNT];
	size_t rules_count_by_type[PM_NODE_TYPE_COUNT];
} rules_by_type_t;

/* Build per-file rules_by_type based on configuration and file path. */
bool build_rules_by_type_for_file(const config_t *cfg, const char *file_path, rules_by_type_t *out);

/* Get (and cache) rules_by_type for a configuration+file path. Returned pointer is owned by the rule manager. */
const rules_by_type_t *get_rules_by_type_for_file(const config_t *cfg, const char *file_path);

/* Free helper for rules_by_type built by caller. */
void free_rules_by_type(rules_by_type_t *rb);

/* Clear internal cache of per-file rule sets. */
void rule_manager_clear_cache(void);

// Visit node for rule checking (legacy - uses global registry-based rules)
bool visit_node(pm_node_t *node, pm_parser_t *parser, pm_list_t *diagnostics, config_t *cfg);

// Visit node with a per-file rules set
bool visit_node_with_rules(pm_node_t *node, pm_parser_t *parser, pm_list_t *diagnostics, config_t *cfg, const rules_by_type_t *rules);

/* Timing helpers for benchmarking */
void rule_manager_reset_timing(void);
void rule_manager_get_timing(uint64_t *time_ns, size_t *calls);

/* Per-rule timing and dump */
void rule_manager_dump_timings(void);

#endif /* LEUKOCYTE_RULE_MANAGER_H */
