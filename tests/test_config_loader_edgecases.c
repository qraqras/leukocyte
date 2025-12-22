// Edge-case tests for config loader merging behavior
#include <assert.h>
#include <stdio.h>
#include <yaml.h>
#include "configs/conversion/loader.h"
#include "configs/config.h"
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
    const rule_registry_entry_t *reg = leuko_get_rule_registry();
    size_t count = leuko_get_rule_registry_count();
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
    leuko_config_t cfg = {0};
    leuko_config_initialize(&cfg);
    const char *yaml1 = "AllCops:\n  Severity: warning\n";
    yaml_document_t *d1 = load_doc(yaml1);
    assert(d1);
    yaml_document_t *docs1[1] = {d1};
    assert(apply_config_docs(docs1, 1, &cfg, NULL));
    size_t idx = find_rule_index("Layout/IndentationConsistency");
    assert(idx != (size_t)-1);
    leuko_rule_config_t *cfgp = leuko_rule_config_get_by_index(&cfg, idx);
    assert(cfgp->severity_level == LEUKO_SEVERITY_WARNING);
    yaml_document_delete(d1);
    free(d1);
    leuko_config_free(&cfg);

    // Test 2: Category overrides AllCops
    leuko_config_t cfg2 = {0};
    leuko_config_initialize(&cfg2);
    const char *yaml2 = "AllCops:\n  Severity: warning\nLayout:\n  Severity: error\n  IndentationConsistency:\n    EnforcedStyle: indented_internal_methods\n";
    yaml_document_t *d2 = load_doc(yaml2);
    assert(d2);
    yaml_document_t *docs2[1] = {d2};
    assert(apply_config_docs(docs2, 1, &cfg2, NULL));
    leuko_rule_config_t *cfgp2 = leuko_rule_config_get_by_index(&cfg2, idx);
    // category should take precedence
    assert(cfgp2->severity_level == LEUKO_SEVERITY_ERROR);
    // enforced style from rule node
    layout_indentation_consistency_config_t *sc = (layout_indentation_consistency_config_t *)cfgp2->specific_config;
    assert(sc->enforced_style == LAYOUT_INDENTATION_CONSISTENCY_ENFORCED_STYLE_INDENTED_INTERNAL_METHODS);
    yaml_document_delete(d2);
    free(d2);
    leuko_config_free(&cfg2);

    // Test 3: Rule Include/Exclude merging order (AllCops->Category->Rule)
    leuko_config_t cfg3 = {0};
    leuko_config_initialize(&cfg3);
    const char *yaml3 = "AllCops:\n  Include: [a]\nLayout:\n  Include: [b]\n  IndentationConsistency:\n    Include: [c]\n";
    yaml_document_t *d3 = load_doc(yaml3);
    assert(d3);
    yaml_document_t *docs3[1] = {d3};
    assert(apply_config_docs(docs3, 1, &cfg3, NULL));
    leuko_rule_config_t *cfgp3 = leuko_rule_config_get_by_index(&cfg3, idx);
    assert(cfgp3->include_count == 3);
    assert(strcmp(cfgp3->include[0], "a") == 0);
    assert(strcmp(cfgp3->include[1], "b") == 0);
    assert(strcmp(cfgp3->include[2], "c") == 0);
    yaml_document_delete(d3);
    free(d3);
    leuko_config_free(&cfg3);

    // Test 4: Type mismatch for Severity (sequence) does not change severity; diagnostics are not emitted
    leuko_config_t cfg4 = {0};
    leuko_config_initialize(&cfg4);
    const char *yaml4 = "AllCops:\n  Severity: warning\nLayout:\n  IndentationConsistency:\n    Severity: [bad]\n";
    yaml_document_t *d4 = load_doc(yaml4);
    assert(d4);
    char *err = NULL;
    yaml_document_t *docs4[1] = {d4};
    assert(apply_config_docs(docs4, 1, &cfg4, &err));
    leuko_rule_config_t *cfgp4 = leuko_rule_config_get_by_index(&cfg4, idx);
    // Because rule-level Severity is not scalar, merged string lookup should fall back to category/allcops -> warning
    assert(cfgp4->severity_level == LEUKO_SEVERITY_WARNING);
    // diagnostics should NOT have an entry (we do not emit diagnostics for non-scalar Severity)
    const char *msg = err;
    assert(msg == NULL);
    free(err);
    yaml_document_delete(d4);
    free(d4);
    leuko_config_free(&cfg4);

    // Test 5: Severity token mapping (refactor -> refactor)
    leuko_config_t cfg_refactor = {0};
    leuko_config_initialize(&cfg_refactor);
    const char *yaml_refactor = "AllCops:\n  Severity: refactor\n";
    yaml_document_t *d_refactor = load_doc(yaml_refactor);
    assert(d_refactor);
    yaml_document_t *docs_refactor[1] = {d_refactor};
    assert(apply_config_docs(docs_refactor, 1, &cfg_refactor, NULL));
    leuko_rule_config_t *cfgp_refactor = leuko_rule_config_get_by_index(&cfg_refactor, idx);
    assert(cfgp_refactor->severity_level == LEUKO_SEVERITY_REFACTOR);
    yaml_document_delete(d_refactor);
    free(d_refactor);
    leuko_config_free(&cfg_refactor);

    // Test 6: Severity token mapping (fatal -> fatal)
    leuko_config_t cfg_fatal = {0};
    leuko_config_initialize(&cfg_fatal);
    const char *yaml_fatal = "AllCops:\n  Severity: fatal\n";
    yaml_document_t *d_fatal = load_doc(yaml_fatal);
    assert(d_fatal);
    yaml_document_t *docs_fatal[1] = {d_fatal};
    assert(apply_config_docs(docs_fatal, 1, &cfg_fatal, NULL));
    leuko_rule_config_t *cfgp_fatal = leuko_rule_config_get_by_index(&cfg_fatal, idx);
    assert(cfgp_fatal->severity_level == LEUKO_SEVERITY_FATAL);
    yaml_document_delete(d_fatal);
    free(d_fatal);
    leuko_config_free(&cfg_fatal);

    // Test 5: Enabled boolean precedence
    leuko_config_t cfg5 = {0};
    leuko_config_initialize(&cfg5);
    const char *yaml5 = "AllCops:\n  Enabled: true\nLayout:\n  IndentationConsistency:\n    Enabled: false\n";
    yaml_document_t *d5 = load_doc(yaml5);
    assert(d5);
    yaml_document_t *docs5[1] = {d5};
    assert(apply_config_docs(docs5, 1, &cfg5, NULL));
    leuko_rule_config_t *cfgp5 = leuko_rule_config_get_by_index(&cfg5, idx);
    assert(cfgp5->enabled == false);
    yaml_document_delete(d5);
    free(d5);
    leuko_config_free(&cfg5);

    // All tests passed
    return 0;
}
