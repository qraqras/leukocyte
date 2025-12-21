#ifndef LEUKOCYTE_CONFIGS_INHERIT_H
#define LEUKOCYTE_CONFIGS_INHERIT_H

#include <stddef.h>
#include "configs/raw_config.h"

/*
 * Resolve `inherit_from` entries from a parsed raw config.
 * - On success returns 0 and sets *out to an allocated array of leuko_raw_config_t* and *out_count to the count.
 *   The caller owns the array and each element and must free them via `leuko_raw_config_list_free`.
 * - On error returns non-zero and sets *err to a short message (caller frees).
 */
int leuko_config_resolve_inherit_from(const leuko_raw_config_t *base, leuko_raw_config_t ***out, size_t *out_count, char **err);

/* Recursively collect parent configs in inheritance order (parents first). Detects cycles and returns error on cycle. */
int leuko_config_collect_inherit_chain(const leuko_raw_config_t *base, leuko_raw_config_t ***out, size_t *out_count, char **err);

/* Free a list returned by leuko_config_resolve_inherit_from or leuko_config_collect_inherit_chain */
void leuko_raw_config_list_free(leuko_raw_config_t **list, size_t count);

#endif /* LEUKOCYTE_CONFIGS_INHERIT_H */
