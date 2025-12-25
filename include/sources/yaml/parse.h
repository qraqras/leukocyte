#ifndef LEUKOCYTE_SOURCES_YAML_PARSE_H
#define LEUKOCYTE_SOURCES_YAML_PARSE_H

#include "configs/compiled_config.h"
#include "configs/common/rule_config.h"
#include "sources/yaml/node.h"

/* Parse a YAML file into a leuko_yaml_node_t. Returns true on success. */
bool leuko_yaml_parse(const char *path, leuko_yaml_node_t **out);

#endif /* LEUKOCYTE_SOURCES_YAML_PARSE_H */
