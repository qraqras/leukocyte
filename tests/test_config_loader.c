// Tests for config loader apply_document
#include <assert.h>
#include <yaml.h>
#include "configs/loader.h"
#include "configs/config.h"
#include "configs/generated_config.h"

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
    // initialize defaults
    config_initialize(NULL);
    const rule_registry_entry_t *reg_all = get_rule_registry();
    size_t reg_count = get_rule_registry_count();
    (void)reg_all;
    (void)reg_count;

    const char *yaml = "Layout:\n  IndentationConsistency:\n    Enabled: false\n    EnforcedStyle: indented_internal_methods\n";
    yaml_document_t *doc = load_doc(yaml);
    assert(doc);
    bool ok = config_apply_document(doc, NULL);
    assert(ok);

    // find registry for indentation rule (first entry)
    const rule_registry_entry_t *reg = get_rule_registry();
    rule_config_t *cfg = config_get_by_index(0);
    assert(cfg);
    // Enabled set to false
    assert(cfg->enabled == false);

    // specific_config enforced_style is checked
    layout_indentation_consistency_config_t *sc = (layout_indentation_consistency_config_t *)cfg->specific_config;
    assert(sc->enforced_style == INDENTATION_CONSISTENCY_ENFORCED_STYLE_INDENTED_INTERNAL_METHODS);

    yaml_document_delete(doc);
    free(doc);
    config_free(NULL);
    return 0;
}
