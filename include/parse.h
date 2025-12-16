#ifndef LEUKOCYTE_PARSE_H
#define LEUKOCYTE_PARSE_H

#include <stdbool.h>
#include "prism.h" /* pm_node_t, pm_parser_t */

/* Parse Ruby file at `filepath` into a Prism AST node.
 * On success returns true and sets `*out_node` and initializes `out_parser`.
 * Ownership: the parser may reference the underlying source buffer; callers
 * should call `pm_parser_free(out_parser)` (and related cleanup) when done.
 */
/* On success returns true, sets `*out_node`, initializes `out_parser`, and
 * sets `*out_source` to the allocated buffer that backs the parser.
 * Caller must call `pm_parser_free(out_parser)` and then `free(*out_source)`.
 */
bool parse_ruby_file(const char *filepath, pm_node_t **out_node, pm_parser_t *out_parser, uint8_t **out_source);

#endif /* LEUKOCYTE_PARSE_H */
