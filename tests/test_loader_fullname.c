#include <assert.h>
#include <yaml.h>
#include "configs/loader.h"
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
    config_t cfg = {0};
    initialize_config(&cfg);

    const char *yaml = "Layout/IndentationConsistency:\n  Enabled: false\n";
    yaml_document_t *doc = load_doc(yaml);
    assert(doc);
    bool ok = apply_config(doc, &cfg, NULL);
    assert(ok);

    leuko_rule_config_t *r = get_rule_config_by_index(&cfg, 0);
    assert(r);
    assert(r->enabled == false);

    yaml_document_delete(doc);
    free(doc);
    free_config(&cfg);
    return 0;
}
