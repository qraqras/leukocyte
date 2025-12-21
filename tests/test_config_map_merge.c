#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "configs/config.h"
#include "configs/loader.h"
#include "configs/raw_config.h"
#include "configs/inherit.h"
#include "configs/yaml_helpers.h"

static int write_file(const char *path, const char *content)
{
    FILE *f = fopen(path, "wb");
    if (!f)
        return 0;
    fwrite(content, 1, strlen(content), f);
    fclose(f);
    return 1;
}

int main(void)
{
    /* parent with nested map */
    write_file("tests/tmp_map_parent.yml", "Layout:\n  SomeRule:\n    Nested:\n      A: 1\n      C: parent\n");
    /* child overrides and adds (relative path) */
    write_file("tests/tmp_map_child.yml", "inherit_from: tmp_map_parent.yml\nLayout:\n  SomeRule:\n    Nested:\n      B: 2\n      C: child\n");

    config_t cfg = {0};
    initialize_config(&cfg);
    char *err = NULL;
    bool ok = load_config_file_into(&cfg, "tests/tmp_map_child.yml", &err);
    assert(ok);

    /* For this test, implement rule-specific apply that reads Nested.C and Nested.A/B via generated config system.
       Rather than wiring into a real rule, we'll use the generated apply by directly invoking merged helper and check its content. */

    /* Use yaml_merge_rule_mapping_multi directly to inspect merged mapping */
    extern yaml_document_t *yaml_merge_rule_mapping_multi(yaml_document_t * *docs, size_t doc_count, const char *full_name, const char *category_name);
    leuko_raw_config_t *base = NULL;
    int rc = leuko_config_load_file("tests/tmp_map_child.yml", &base, &err);
    if (rc != 0)
    {
        fprintf(stderr, "leuko_config_load_file failed: %s\n", err ? err : "(no err)");
        if (err)
            free(err);
        return 1;
    }
    (void)base;
    (void)err;
    leuko_raw_config_t **parents = NULL;
    size_t parent_count = 0;
    rc = leuko_config_collect_inherit_chain(base, &parents, &parent_count, &err);
    if (rc != 0)
    {
        fprintf(stderr, "collect_inherit_chain failed: %s\n", err ? err : "(no err)");
        if (err)
            free(err);
        leuko_raw_config_free(base);
        return 1;
    }
    size_t doc_count = parent_count + 1;
    yaml_document_t **docs = calloc(doc_count, sizeof(yaml_document_t *));
    for (size_t i = 0; i < parent_count; i++)
        docs[i] = parents[i]->doc;
    docs[parent_count] = base->doc;

    yaml_document_t *merged = yaml_merge_rule_mapping_multi(docs, doc_count, "Layout/SomeRule", "Layout");
    assert(merged != NULL);
    yaml_node_t *root = yaml_document_get_root_node(merged);
    assert(root != NULL && root->type == YAML_MAPPING_NODE);
    assert(root != NULL && root->type == YAML_MAPPING_NODE);
    char *cval = yaml_get_mapping_scalar_value(merged, root, "Nested");
    (void)cval; /* Nested is a mapping, so scalar lookup returns NULL */

    /* Inspect nested mapping by retrieving 'Nested' node */
    yaml_node_t *nested = yaml_get_mapping_node(merged, root, "Nested");
    assert(nested != NULL);
    char *a = yaml_get_mapping_scalar_value(merged, nested, "A");
    char *b = yaml_get_mapping_scalar_value(merged, nested, "B");
    char *c = yaml_get_mapping_scalar_value(merged, nested, "C");
    assert(a && strcmp(a, "1") == 0);
    assert(b && strcmp(b, "2") == 0);
    assert(c && strcmp(c, "child") == 0);

    yaml_document_delete(merged);
    free(merged);
    leuko_raw_config_list_free(parents, parent_count);
    leuko_raw_config_free(base);
    free(docs);
    free_config(&cfg);
    return 0;
}
