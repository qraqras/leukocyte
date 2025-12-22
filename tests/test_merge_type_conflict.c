#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "configs/discovery/raw_config.h"
#include "configs/discovery/inherit.h"
#include "configs/conversion/merge.h"
#include "configs/conversion/yaml_helpers.h"

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
    /* parent: scalar */
    write_file("tests/tmp_parent_typeconf.yml", "Layout:\n  SomeMap: \"scalar-parent\"\n");
    /* child: mapping */
    write_file("tests/tmp_child_typeconf.yml", "inherit_from: tests/tmp_parent_typeconf.yml\nLayout:\n  SomeMap:\n    foo: bar\n");

    char *err = NULL;
    /* Load base (child) */
    leuko_raw_config_t *base = NULL;
    int rc = leuko_config_load_file("tests/tmp_child_typeconf.yml", &base, &err);
    printf("[test] load base rc=%d base=%p err=%p\n", rc, (void *)base, (void *)err);
    assert(rc == 0 && base != NULL);

    /* Resolve inherit chain (parents only) */
    leuko_raw_config_t **parents = NULL;
    size_t parent_count = 0;
    rc = leuko_config_collect_inherit_chain(base, &parents, &parent_count, &err);
    printf("[test] collect_inherit_chain rc=%d parent_count=%zu\n", rc, parent_count);
    assert(rc == 0);

    /* Build docs array parent-first, then base */
    size_t doc_count = parent_count + 1;
    yaml_document_t **docs = calloc(doc_count, sizeof(yaml_document_t *));
    assert(docs);
    for (size_t i = 0; i < parent_count; i++)
        docs[i] = parents[i]->doc;
    docs[parent_count] = base->doc;

    yaml_document_t *merged = yaml_merge_documents_multi(docs, doc_count);
    free(docs);
    leuko_raw_config_list_free(parents, parent_count);
    leuko_raw_config_free(base);
    assert(merged != NULL);

    yaml_node_t *root = yaml_document_get_root_node(merged);
    assert(root && root->type == YAML_MAPPING_NODE);

    yaml_node_t *layout = yaml_get_mapping_node(merged, root, "Layout");
    assert(layout && layout->type == YAML_MAPPING_NODE);

    yaml_node_t *sm = yaml_get_mapping_node(merged, layout, "SomeMap");
    printf("[test] somemap=%p type=%d\n", (void *)sm, sm ? sm->type : -1);
    assert(sm && sm->type == YAML_MAPPING_NODE);

    char *v = yaml_get_mapping_scalar_value(merged, sm, "foo");
    printf("[test] foo=%s\n", v ? v : "(null)");
    assert(v && strcmp(v, "bar") == 0);
    free(v);

    yaml_document_delete(merged);
    free(merged);
    printf("[test] done\n");
    return 0;
}
