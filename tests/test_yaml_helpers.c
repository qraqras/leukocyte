// Tests for YAML merged helpers
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
    const char *yaml = "AllCops:\n  Severity: warning\nLayout:\n  IndentationConsistency:\n    EnforcedStyle: indented_internal_methods\n    Include: [a, b]\n";
    yaml_document_t *doc = load_doc(yaml);
    assert(doc);
    yaml_node_t *root = yaml_document_get_root_node(doc);
    // root is mapping; find Layout node
    yaml_node_t *layout = NULL;
    for (yaml_node_pair_t *p = root->data.mapping.pairs.start; p < root->data.mapping.pairs.top; p++)
    {
        yaml_node_t *k = yaml_document_get_node(doc, p->key);
        if (strcmp((char *)k->data.scalar.value, "Layout") == 0)
            layout = yaml_document_get_node(doc, p->value);
    }
    assert(layout);
    // Category node is mapping under Layout
    yaml_node_t *indent = NULL;
    for (yaml_node_pair_t *p = layout->data.mapping.pairs.start; p < layout->data.mapping.pairs.top; p++)
    {
        yaml_node_t *k = yaml_document_get_node(doc, p->key);
        if (strcmp((char *)k->data.scalar.value, "IndentationConsistency") == 0)
            indent = yaml_document_get_node(doc, p->value);
    }
    assert(indent);

    char val[128];
    bool ok = yaml_get_merged_string(doc, indent, layout, root, "EnforcedStyle", val);
    assert(ok == true && strcmp(val, "indented_internal_methods") == 0);

    bool b = false;
    ok = yaml_get_merged_bool(doc, indent, layout, root, "NotExist", &b);
    assert(ok == false);

    /* Additional boolean parsing checks */
    const char *yaml_bool = "AllCops:\n  Enabled: false\n";
    yaml_document_t *doc_bool = load_doc(yaml_bool);
    assert(doc_bool);
    yaml_node_t *root_bool = yaml_document_get_root_node(doc_bool);
    // find AllCops mapping under root_bool
    yaml_node_t *allcops_bool = NULL;
    for (yaml_node_pair_t *p = root_bool->data.mapping.pairs.start; p < root_bool->data.mapping.pairs.top; p++)
    {
        yaml_node_t *k = yaml_document_get_node(doc_bool, p->key);
        yaml_node_t *v = yaml_document_get_node(doc_bool, p->value);
        if (k && k->type == YAML_SCALAR_NODE && strcmp((char *)k->data.scalar.value, "AllCops") == 0)
            allcops_bool = v;
    }
    assert(allcops_bool);

    bool out = true;
    ok = yaml_get_merged_bool(doc_bool, NULL, NULL, allcops_bool, "Enabled", &out);
    assert(ok == true && out == false);

    /* various accepted true/false tokens (case-insensitive) */
    const char *yaml_true_variants[] = {"Enabled: True\n", "Enabled: TRUE\n", "Enabled: yes\n", "Enabled: On\n", "Enabled: 1\n"};
    for (size_t i = 0; i < sizeof(yaml_true_variants) / sizeof(yaml_true_variants[0]); i++)
    {
        yaml_document_t *d = load_doc(yaml_true_variants[i]);
        assert(d);
        yaml_node_t *r = yaml_document_get_root_node(d);
        bool val = false;
        ok = yaml_get_merged_bool(d, NULL, NULL, r, "Enabled", &val);
        assert(ok == true && val == true);
        yaml_document_delete(d);
        free(d);
    }

    const char *yaml_false_variants[] = {"Enabled: False\n", "Enabled: NO\n", "Enabled: 0\n", "Enabled: off\n"};
    for (size_t i = 0; i < sizeof(yaml_false_variants) / sizeof(yaml_false_variants[0]); i++)
    {
        yaml_document_t *d = load_doc(yaml_false_variants[i]);
        assert(d);
        yaml_node_t *r = yaml_document_get_root_node(d);
        bool val = true;
        ok = yaml_get_merged_bool(d, NULL, NULL, r, "Enabled", &val);
        assert(ok == true && val == false);
        yaml_document_delete(d);
        free(d);
    }

    yaml_document_delete(doc_bool);
    free(doc_bool);

    size_t count = 0;
    fprintf(stderr, "debug: about to get sequence\n");
    char **arr = NULL;
    ok = yaml_get_merged_sequence(doc, indent, layout, root, "Include", &arr, &count);
    fprintf(stderr, "debug: got sequence ok=%d count=%zu arr=%p\n", ok, count, (void *)arr);
    assert(ok == true && arr && count == 2);
    free(arr[0]);
    free(arr[1]);
    free(arr);

    yaml_document_delete(doc);
    free(doc);
    return 0;
}
