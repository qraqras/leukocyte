#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "configs/compiled_config.h"
#include "sources/yaml/merge.h"

int main(void)
{
    const char *td = "tests/tmp_norm";
    system("rm -rf tests/tmp_norm && mkdir -p tests/tmp_norm");
    FILE *f = fopen("tests/tmp_norm/.rubocop.yml", "w");
    if (!f)
        return 2;
    fprintf(f, "Layout:\n  Exclude:\n    - \"vendor/**/*\"\n  IndentationConsistency:\n    EnforcedStyle: indented_internal_methods\n  UnknownRule:\n    SomeKey: true\n");
    fclose(f);

    leuko_compiled_config_t *c = leuko_compiled_config_build(td, NULL);
    if (!c)
    {
        fprintf(stderr, "build failed\n");
        return 2;
    }

    const yaml_document_t *doc = leuko_compiled_config_merged_doc(c);
    if (!doc)
    {
        fprintf(stderr, "no merged_doc\n");
        leuko_compiled_config_unref(c);
        return 2;
    }

    leuko_yaml_node_t *root = leuko_yaml_node_from_document((yaml_document_t *)doc);
    if (!root)
    {
        fprintf(stderr, "conversion failed\n");
        leuko_compiled_config_unref(c);
        return 2;
    }

    leuko_yaml_normalize_rule_keys(root);

    const char *val = leuko_yaml_node_get_rule_mapping_scalar(root, "Layout/IndentationConsistency", "EnforcedStyle");
    if (!val || strcmp(val, "indented_internal_methods") != 0)
    {
        fprintf(stderr, "normalize failed: got '%s'\n", val ? val : "(null)");
        leuko_yaml_node_free(root);
        leuko_compiled_config_unref(c);
        return 2;
    }

    /* Ensure unknown subkey is not expanded */
    leuko_yaml_node_t *unk = leuko_yaml_node_get_rule_mapping(root, "Layout/UnknownRule");
    if (unk)
    {
        fprintf(stderr, "unexpected expansion of unknown rule\n");
        leuko_yaml_node_free(root);
        leuko_compiled_config_unref(c);
        return 2;
    }

    /* Materialization should have created effective_config (non-NULL) */
    if (!leuko_compiled_config_rules(c))
    {
        fprintf(stderr, "no effective_config\n");
        leuko_yaml_node_free(root);
        leuko_compiled_config_unref(c);
        return 2;
    }

    leuko_yaml_node_free(root);
    leuko_compiled_config_unref(c);
    printf("OK\n");
    return 0;
}
