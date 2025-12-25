#ifndef LEUKOCYTE_SOURCES_JSON_PARSE_H
#define LEUKOCYTE_SOURCES_JSON_PARSE_H

#include <stddef.h>
#include "sources/node.h"

bool leuko_json_parse(const char *path, leuko_node_t **out);

#endif /* LEUKOCYTE_SOURCES_JSON_PARSE_H */
