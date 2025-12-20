/* Context passed to rule handlers: bundle config + auxiliary data rules may need */
#ifndef LEUKOCYTE_RULE_CONTEXT_H
#define LEUKOCYTE_RULE_CONTEXT_H

#include "prism.h"
#include "io/processed_source.h"

/* Forward-declare config type to avoid circular include */
typedef struct config_s config_t; // Forward-declare rules_by_type to avoid include cycle with rule_manager
typedef struct rules_by_type_s rules_by_type_t;
typedef struct rule_context_s
{
    const config_t *cfg;          /* Rule configuration (read-only) */
    const rules_by_type_t *rules; /* Optional per-file rule set (may be NULL) */
    leuko_processed_source_t *ps; /* Optional processed source (may be NULL); non-const so handlers can use cache */
    pm_parser_t *parser;          /* Parser pointer */
    pm_list_t *diagnostics;       /* Diagnostics list to append to */
    pm_node_t *parent;            /* Direct parent node if available (may be NULL) */

    /* Ancestors stack: most-recent ancestor is at ancestors[ancestors_count - 1] */
    pm_node_t **ancestors;
    size_t ancestors_count;
    size_t ancestors_cap;
} rule_context_t;

#endif /* LEUKOCYTE_RULE_CONTEXT_H */
