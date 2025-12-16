#include <assert.h>
#include <stdio.h>
#include <yaml.h>

#include "configs/yaml_helpers.h"

static yaml_document_t *load_doc(const char *input)
{
    yaml_parser_t parser;
    yaml_document_t *doc = malloc(sizeof(*doc));
    yaml_parser_initialize(&parser);
    yaml_parser_set_input_string(&parser, (const unsigned char *)input, strlen(input));
    if (!yaml_parser_load(&parser, doc))
    {
        yaml_parser_delete(&parser);
        free(doc);
        return NULL;
    }
    yaml_parser_delete(&parser);
    return doc;
}

int main(void)
{
    /* Key lookup is case-sensitive: 'SomeKey' exists but 'somekey' should not. */
    const char *yaml1 = "SomeKey: value\n";
    yaml_document_t *doc1 = load_doc(yaml1);
    assert(doc1);
    yaml_node_t *root1 = yaml_document_get_root_node(doc1);

    char buf[64];
    bool found = yaml_get_merged_string(doc1, NULL, NULL, root1, "SomeKey", buf);
    assert(found == true && strcmp(buf, "value") == 0);

    found = yaml_get_merged_string(doc1, NULL, NULL, root1, "somekey", buf);
    assert(found == false);

    yaml_document_delete(doc1);
    free(doc1);

    /* RuboCop behavior: boolean parsing is case-insensitive for boolean tokens. */
    const char *yaml2 = "Enabled: True\n";
    yaml_document_t *doc2 = load_doc(yaml2);
    assert(doc2);
    yaml_node_t *root2 = yaml_document_get_root_node(doc2);

    bool out = false;
    bool ok = yaml_get_merged_bool(doc2, NULL, NULL, root2, "Enabled", &out);
    /* 'True' should be parsed as true under RuboCop semantics */
    assert(ok == true);
    assert(out == true);

    /* Lowercase 'true' must be parsed as true */
    const char *yaml3 = "AllCops:\n  Enabled: true\n";
    yaml_document_t *doc3 = load_doc(yaml3);
    assert(doc3);
    yaml_node_t *root3 = yaml_document_get_root_node(doc3);
    // find AllCops mapping under root3
    yaml_node_t *allcops_node = NULL;
    if (root3->type == YAML_MAPPING_NODE)
    {
        for (yaml_node_pair_t *p = root3->data.mapping.pairs.start; p < root3->data.mapping.pairs.top; p++)
        {
            yaml_node_t *k = yaml_document_get_node(doc3, p->key);
            yaml_node_t *v = yaml_document_get_node(doc3, p->value);
            if (k && k->type == YAML_SCALAR_NODE && strcmp((char *)k->data.scalar.value, "AllCops") == 0)
                allcops_node = v;
        }
    }
    assert(allcops_node);
    out = false;
    ok = yaml_get_merged_bool(doc3, NULL, NULL, allcops_node, "Enabled", &out);
    assert(ok == true);
    assert(out == true);

    yaml_document_delete(doc2);
    free(doc2);
    yaml_document_delete(doc3);
    free(doc3);

    return 0;
}
