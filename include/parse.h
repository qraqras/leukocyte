#ifndef LEUKOCYTE_PARSE_H
#define LEUKOCYTE_PARSE_H

#include <stdbool.h>

#include "prism.h"

bool leuko_parse_ruby_file(const char *filepath, pm_node_t **out_node, pm_parser_t *out_parser, uint8_t **out_source);

#endif /* LEUKOCYTE_PARSE_H */
