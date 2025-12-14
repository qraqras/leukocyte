#ifndef LEUKOCYTE_H
#define LEUKOCYTE_H

#include <stdbool.h>
#include <stddef.h>
#include "prism.h" // pm_node_t, pm_parser_t

// Function to parse Ruby code using Prism
bool parse_ruby_file(const char *filepath, pm_node_t **out_node, pm_parser_t *out_parser);

#endif // LEUKOCYTE_H
