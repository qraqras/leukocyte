#include <assert.h>
#include <yaml.h>
#include "configs/conversion/loader.h"
#include "configs/rule_config.h"
#include "configs/config.h"

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
    leuko_config_t cfg = {0};
    leuko_config_initialize(&cfg);

    const char *yaml = "Layout/IndentationConsistency:\n  Enabled: false\n";
    yaml_document_t *doc = load_doc(yaml);
    assert(doc);
    yaml_document_t *docs[1] = {doc};
    bool ok = leuko_config_apply_docs(&cfg, docs, 1, NULL);
    assert(ok);

    leuko_rule_config_t *r = leuko_rule_config_get_by_index(&cfg, 0);
    assert(r);
    assert(r->enabled == false);

    yaml_document_delete(doc);
    free(doc);
    leuko_config_free(&cfg);
    return 0;
}
