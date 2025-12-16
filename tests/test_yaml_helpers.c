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

    char *s = yaml_get_merged_string(doc, indent, layout, root, "EnforcedStyle");
    assert(s && strcmp(s, "indented_internal_methods") == 0);
    free(s);

    int b = 0;
    int ok = yaml_get_merged_bool(doc, indent, layout, root, "NotExist", &b);
    assert(ok == 0);

    size_t count = 0;
    char **arr = yaml_get_merged_sequence(doc, indent, layout, root, "Include", &count);
    assert(arr && count == 2);
    free(arr[0]);
    free(arr[1]);
    free(arr);

    yaml_document_delete(doc);
    free(doc);
    return 0;
}
