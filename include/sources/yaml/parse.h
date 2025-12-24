#ifndef LEUKO_SOURCES_YAML_PARSE_H
#define LEUKO_SOURCES_YAML_PARSE_H

#include <stdbool.h>
#include <yaml.h>

/* Parse YAML file at `path`. On success returns true and sets *out_doc to a
 * heap-allocated yaml_document_t (caller must call yaml_document_delete and free()).
 */
bool leuko_yaml_parse(const char *path, yaml_document_t **out_doc);

#endif /* LEUKO_SOURCES_YAML_PARSE_H */
