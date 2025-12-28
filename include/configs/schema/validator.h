#ifndef LEUKO_CONFIGS_SCHEMA_VALIDATOR_H
#define LEUKO_CONFIGS_SCHEMA_VALIDATOR_H

#include <stddef.h>
#include "configs/common/rule_config.h" /* for leuko_node_t forward */

typedef struct leuko_schema_diag_s
{
    char *file;    /* source filename (if available) */
    char *path;    /* path inside config (e.g., "Layout/IndentationWidth/Width") */
    char *message; /* human-readable diagnostic */
} leuko_schema_diag_t;

void leuko_schema_diag_free(leuko_schema_diag_t *d);

/* Validate the merged node for Layout category and produce diagnostics.
 * The diagnostics array is allocated and returned via out_diags and must be
 * freed by the caller using leuko_schema_diag_free on each element and
 * free(out_diags).
 * Returns true if no diagnostics (valid), false otherwise.
 */
bool leuko_schema_validate_layout(const leuko_node_t *merged, leuko_schema_diag_t ***out_diags, size_t *out_count);

#endif /* LEUKO_CONFIGS_SCHEMA_VALIDATOR_H */
