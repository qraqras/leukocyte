// Edge-case tests for config loader merging behavior
#include <assert.h>
#include <stdio.h>
#include <yaml.h>
#include "configs/loader.h"
#include "configs/generated_config.h"
#include "rule_registry.h"
#include "configs/layout/indentation_consistency.h"

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

static size_t find_rule_index(const char *name)
{
    const rule_registry_entry_t *reg = get_rule_registry();
    size_t count = get_rule_registry_count();
    for (size_t i = 0; i < count; i++)
    {
        if (strcmp(reg[i].full_name, name) == 0)
            return i;
    }
    return (size_t)-1;
}

int main(void)
{
    // Test 1: AllCops Severity -> applied to rule
    config_t cfg = {0};
    initialize_config(&cfg);
    const char *yaml1 = "AllCops:\n  Severity: warning\n";
    yaml_document_t *d1 = load_doc(yaml1);
    assert(d1);
    assert(apply_config(d1, &cfg, NULL));
    size_t idx = find_rule_index("Layout/IndentationConsistency");
    assert(idx != (size_t)-1);
    rule_config_t *cfgp = get_rule_config_by_index(&cfg, idx);
    assert(cfgp->severity_level == SEVERITY_WARNING);
    yaml_document_delete(d1);
    free(d1);
    free_config(&cfg);

    // Test 2: Category overrides AllCops
    config_t cfg2 = {0};
    initialize_config(&cfg2);
    const char *yaml2 = "AllCops:\n  Severity: warning\nLayout:\n  Severity: error\n  IndentationConsistency:\n    EnforcedStyle: indented_internal_methods\n";
    yaml_document_t *d2 = load_doc(yaml2);
    assert(d2);
    assert(apply_config(d2, &cfg2, NULL));
    rule_config_t *cfgp2 = get_rule_config_by_index(&cfg2, idx);
    // category should take precedence
    assert(cfgp2->severity_level == SEVERITY_ERROR);
    // enforced style from rule node
    layout_indentation_consistency_config_t *sc = (layout_indentation_consistency_config_t *)cfgp2->specific_config;
    assert(sc->enforced_style == LAYOUT_INDENTATION_CONSISTENCY_ENFORCED_STYLE_INDENTED_INTERNAL_METHODS);
    yaml_document_delete(d2);
    free(d2);
    free_config(&cfg2);

    // Test 3: Rule Include/Exclude merging order (AllCops->Category->Rule)
    config_t cfg3 = {0};
    initialize_config(&cfg3);
    const char *yaml3 = "AllCops:\n  Include: [a]\nLayout:\n  Include: [b]\n  IndentationConsistency:\n    Include: [c]\n";
    yaml_document_t *d3 = load_doc(yaml3);
    assert(d3);
    assert(apply_config(d3, &cfg3, NULL));
    rule_config_t *cfgp3 = get_rule_config_by_index(&cfg3, idx);
    assert(cfgp3->include_count == 3);
    assert(strcmp(cfgp3->include[0], "a") == 0);
    assert(strcmp(cfgp3->include[1], "b") == 0);
    assert(strcmp(cfgp3->include[2], "c") == 0);
    yaml_document_delete(d3);
    free(d3);
    free_config(&cfg3);

    // Test 4: Type mismatch for Severity (sequence) does not change severity; diagnostics are not emitted
    config_t cfg4 = {0};
    initialize_config(&cfg4);
    const char *yaml4 = "AllCops:\n  Severity: warning\nLayout:\n  IndentationConsistency:\n    Severity: [bad]\n";
    yaml_document_t *d4 = load_doc(yaml4);
    assert(d4);
    char *err = NULL;
    assert(apply_config(d4, &cfg4, &err));
    rule_config_t *cfgp4 = get_rule_config_by_index(&cfg4, idx);
    // Because rule-level Severity is not scalar, merged string lookup should fall back to category/allcops -> warning
    assert(cfgp4->severity_level == SEVERITY_WARNING);
    // diagnostics should NOT have an entry (we do not emit diagnostics for non-scalar Severity)
    const char *msg = err;
    assert(msg == NULL);
    free(err);
    yaml_document_delete(d4);
    free(d4);
    free_config(&cfg4);

    // Test 5: Severity token mapping (refactor -> refactor)
    config_t cfg_refactor = {0};
    initialize_config(&cfg_refactor);
    const char *yaml_refactor = "AllCops:\n  Severity: refactor\n";
    yaml_document_t *d_refactor = load_doc(yaml_refactor);
    assert(d_refactor);
    assert(apply_config(d_refactor, &cfg_refactor, NULL));
    rule_config_t *cfgp_refactor = get_rule_config_by_index(&cfg_refactor, idx);
    assert(cfgp_refactor->severity_level == SEVERITY_REFACTOR);
    yaml_document_delete(d_refactor);
    free(d_refactor);
    free_config(&cfg_refactor);

    // Test 6: Severity token mapping (fatal -> fatal)
    config_t cfg_fatal = {0};
    initialize_config(&cfg_fatal);
    const char *yaml_fatal = "AllCops:\n  Severity: fatal\n";
    yaml_document_t *d_fatal = load_doc(yaml_fatal);
    assert(d_fatal);
    assert(apply_config(d_fatal, &cfg_fatal, NULL));
    rule_config_t *cfgp_fatal = get_rule_config_by_index(&cfg_fatal, idx);
    assert(cfgp_fatal->severity_level == SEVERITY_FATAL);
    yaml_document_delete(d_fatal);
    free(d_fatal);
    free_config(&cfg_fatal);

    // Test 5: Enabled boolean precedence
    config_t cfg5 = {0};
    initialize_config(&cfg5);
    const char *yaml5 = "AllCops:\n  Enabled: true\nLayout:\n  IndentationConsistency:\n    Enabled: false\n";
    yaml_document_t *d5 = load_doc(yaml5);
    assert(d5);
    assert(apply_config(d5, &cfg5, NULL));
    rule_config_t *cfgp5 = get_rule_config_by_index(&cfg5, idx);
    assert(cfgp5->enabled == false);
    yaml_document_delete(d5);
    free(d5);
    free_config(&cfg5);

    // All tests passed
    return 0;
}
