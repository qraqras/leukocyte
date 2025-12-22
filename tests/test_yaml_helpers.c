// Tests for YAML merged helpers
#include <assert.h>
#include <stdio.h>
#include <yaml.h>

#include "configs/loader/yaml_helpers.h"

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

    /* Use multi helpers (single doc in docs array) */
    yaml_document_t *docs1[1] = {doc};

    char *sval = NULL;
    bool ok = yaml_get_merged_rule_scalar_multi(docs1, 1, "Layout/IndentationConsistency", "Layout", "IndentationConsistency", "EnforcedStyle", &sval);
    assert(ok == true && sval && strcmp(sval, "indented_internal_methods") == 0);
    free(sval);

    char *snot = NULL;
    ok = yaml_get_merged_rule_scalar_multi(docs1, 1, "Layout/IndentationConsistency", "Layout", "IndentationConsistency", "NotExist", &snot);
    assert(ok == false && snot == NULL);

    /* Additional boolean parsing checks */
    const char *yaml_bool = "AllCops:\n  Enabled: false\n";
    yaml_document_t *doc_bool = load_doc(yaml_bool);
    assert(doc_bool);

    yaml_document_t *docs2[1] = {doc_bool};
    bool out = true;
    ok = yaml_get_merged_rule_bool_multi(docs2, 1, "Layout/IndentationConsistency", "Layout", "IndentationConsistency", "Enabled", &out);
    assert(ok == true && out == false);

    /* various accepted true/false tokens (case-insensitive) */
    const char *yaml_true_variants[] = {"Enabled: True\n", "Enabled: TRUE\n", "Enabled: yes\n", "Enabled: On\n", "Enabled: 1\n"};
    for (size_t i = 0; i < sizeof(yaml_true_variants) / sizeof(yaml_true_variants[0]); i++)
    {
        yaml_document_t *d = load_doc(yaml_true_variants[i]);
        assert(d);
        yaml_document_t *docs3[1] = {d};
        bool val = false;
        ok = yaml_get_merged_rule_bool_multi(docs3, 1, "Layout/IndentationConsistency", "Layout", "IndentationConsistency", "Enabled", &val);
        assert(ok == true && val == true);
        yaml_document_delete(d);
        free(d);
    }

    const char *yaml_false_variants[] = {"Enabled: False\n", "Enabled: NO\n", "Enabled: 0\n", "Enabled: off\n"};
    for (size_t i = 0; i < sizeof(yaml_false_variants) / sizeof(yaml_false_variants[0]); i++)
    {
        yaml_document_t *d = load_doc(yaml_false_variants[i]);
        assert(d);
        yaml_document_t *docs4[1] = {d};
        bool val = true;
        ok = yaml_get_merged_rule_bool_multi(docs4, 1, "Layout/IndentationConsistency", "Layout", "IndentationConsistency", "Enabled", &val);
        assert(ok == true && val == false);
        yaml_document_delete(d);
        free(d);
    }

    yaml_document_delete(doc_bool);
    free(doc_bool);

    size_t count = 0;
    char **arr = NULL;
    ok = yaml_get_merged_rule_sequence_multi(docs1, 1, "Layout/IndentationConsistency", "Layout", "IndentationConsistency", "Include", &arr, &count);
    assert(ok == true && arr && count == 2);
    free(arr[0]);
    free(arr[1]);
    free(arr);

    yaml_document_delete(doc);
    free(doc);
    return 0;
}
