#ifndef LEUKOCYTE_CONFIGS_MERGE_H
#define LEUKOCYTE_CONFIGS_MERGE_H

#include <yaml.h>

/* Merge entire documents (root mappings) parent-first into a single document. Returns newly allocated yaml_document_t* or NULL. */
yaml_document_t *yaml_merge_documents_multi(yaml_document_t **docs, size_t doc_count);

#endif /* LEUKOCYTE_CONFIGS_MERGE_H */
