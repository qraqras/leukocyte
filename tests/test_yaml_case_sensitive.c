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
    /* Key lookup is case-sensitive: 'SomeKey' exists but 'somekey' should not. */
    const char *yaml1 = "SomeKey: value\n";
    yaml_document_t *doc1 = load_doc(yaml1);
    assert(doc1);
    yaml_node_t *root1 = yaml_document_get_root_node(doc1);

    char buf[64];
    yaml_document_t *docs1[1] = {doc1};
    char *s = NULL;
    bool found = yaml_get_merged_rule_scalar_multi(docs1, 1, "SomeKey", NULL, "SomeKey", "SomeKey", &s);
    assert(found == true && s && strcmp(s, "value") == 0);
    free(s);

    char *s2 = NULL;
    found = yaml_get_merged_rule_scalar_multi(docs1, 1, "SomeKey", NULL, "SomeKey", "somekey", &s2);
    assert(found == false && s2 == NULL);

    yaml_document_delete(doc1);
    free(doc1);

    /* RuboCop behavior: boolean parsing is case-insensitive for boolean tokens. */
    const char *yaml2 = "Enabled: True\n";
    yaml_document_t *doc2 = load_doc(yaml2);
    assert(doc2);
    yaml_document_t *docs2[1] = {doc2};

    bool out = false;
    bool ok = yaml_get_merged_rule_bool_multi(docs2, 1, "SomeRule", NULL, "SomeRule", "Enabled", &out);
    /* 'True' should be parsed as true under RuboCop semantics */
    assert(ok == true);
    assert(out == true);

    /* Lowercase 'true' must be parsed as true */
    const char *yaml3 = "AllCops:\n  Enabled: true\n";
    yaml_document_t *doc3 = load_doc(yaml3);
    assert(doc3);
    yaml_document_t *docs3[1] = {doc3};

    out = false;
    ok = yaml_get_merged_rule_bool_multi(docs3, 1, "SomeRule", NULL, "SomeRule", "Enabled", &out);
    assert(ok == true);
    assert(out == true);

    yaml_document_delete(doc2);
    free(doc2);
    yaml_document_delete(doc3);
    free(doc3);

    return 0;
}
