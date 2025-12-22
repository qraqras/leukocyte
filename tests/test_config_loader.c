// Tests for config loader apply_document
#include <assert.h>
#include <yaml.h>
#include "configs/loader/loader.h"
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
    // initialize a local config and apply document to it
    config_t cfg = {0};
    initialize_config(&cfg);

    const char *yaml = "Layout:\n  IndentationConsistency:\n    Enabled: false\n    EnforcedStyle: indented_internal_methods\n";
    yaml_document_t *doc = load_doc(yaml);
    assert(doc);
    bool ok = apply_config(doc, &cfg, NULL);
    assert(ok);

    // check indentation rule (index 0)
    leuko_rule_config_t *r = get_rule_config_by_index(&cfg, 0);
    assert(r);
    // Enabled set to false
    assert(r->enabled == false);

    // specific_config enforced_style is checked
    layout_indentation_consistency_config_t *sc = (layout_indentation_consistency_config_t *)r->specific_config;
    assert(sc->enforced_style == LAYOUT_INDENTATION_CONSISTENCY_ENFORCED_STYLE_INDENTED_INTERNAL_METHODS);

    yaml_document_delete(doc);
    free(doc);
    free_config(&cfg);
    return 0;
}
