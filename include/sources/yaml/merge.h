#include <stdbool.h>
#include <yaml.h>

#ifndef LEUKO_SOURCES_YAML_MERGE_H
#define LEUKO_SOURCES_YAML_MERGE_H

/* Merge parent and child yaml documents into a newly allocated document.
 * Behavior (prototype):
 * - For top-level mapping key "AllCops" and nested keys "Include"/"Exclude"
 *   that are sequences of scalars, the merged document concatenates parent then child
 *   sequence items (child appends after parent).
 * - For other keys, child's value overrides parent's (not implemented in detail in prototype).
 * The result is parsed into a new heap-allocated yaml_document_t and stored in *out_merged.
 * Returns true on success.
 */
bool leuko_yaml_merge_documents(yaml_document_t *parent, yaml_document_t *child, yaml_document_t **out_merged);

#endif /* LEUKO_SOURCES_YAML_MERGE_H */
