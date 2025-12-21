#ifndef LEUKOCYTE_CONFIGS_MERGE_H
#define LEUKOCYTE_CONFIGS_MERGE_H

#include <yaml.h>

/* Build a merged mapping document for a given rule across parent-first docs.
 * Returns a newly allocated yaml_document_t* (caller must call yaml_document_delete and free the struct)
 * or NULL if no merged mapping was found.
 */
yaml_document_t *yaml_merge_rule_mapping_multi(yaml_document_t **docs, size_t doc_count, const char *full_name, const char *category_name);

#endif /* LEUKOCYTE_CONFIGS_MERGE_H */
