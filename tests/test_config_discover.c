#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "configs/discover.h"
#include "configs/raw_config.h"
/* Access yaml helper helpers used in assertions */
#include "configs/yaml_helpers.h"

static int write_file(const char *path, const char *content)
{
    FILE *f = fopen(path, "wb");
    if (!f)
        return 0;
    fwrite(content, 1, strlen(content), f);
    fclose(f);
    return 1;
}

int main(void)
{
    /* Setup directories and files */
    /* Ensure directories exist */
    mkdir("tests/discover_parent", 0755);
    mkdir("tests/discover_parent/child", 0755);
    if (!write_file("tests/discover_parent/.rubocop.yml", "root: true\nAllCops:\n  Exclude:\n    - foo\n"))
    {
        fprintf(stderr, "failed to write parent config\n");
        return 1;
    }
    if (!write_file("tests/discover_parent/child/.rubocop.yml", "Layout:\n  IndentationConsistency:\n    Enabled: false\n"))
    {
        fprintf(stderr, "failed to write child config\n");
        return 1;
    }

    leuko_raw_config_t *cfg = NULL;
    char *err = NULL;

    /* Discover from a file path inside child */
    fprintf(stderr, "discover: starting\n");
    int rc = leuko_config_discover_for_file("tests/discover_parent/child/file.rb", NULL, &cfg, &err);
    fprintf(stderr, "discover: returned rc=%d cfg=%p err=%p\n", rc, (void *)cfg, (void *)err);
    assert(rc == 0);
    assert(cfg != NULL);

    /* Check merged content: child should override parent for specific keys */
    yaml_node_t *root = yaml_document_get_root_node(cfg->doc);
    assert(root);
    /* Layout/IndentationConsistency Enabled should be false from child */
    yaml_node_t *layout = yaml_get_mapping_node(cfg->doc, root, "Layout");
    assert(layout != NULL);
    yaml_node_t *rule = yaml_get_mapping_node(cfg->doc, layout, "IndentationConsistency");
    assert(rule != NULL);
    char *enabled = yaml_get_mapping_scalar_value(cfg->doc, rule, "Enabled");
    assert(enabled != NULL && strcmp(enabled, "false") == 0);

    /* AllCops existed in parent; ensure it is present */
    yaml_node_t *allcops = yaml_get_mapping_node(cfg->doc, root, "AllCops");
    assert(allcops != NULL);

    leuko_raw_config_free(cfg);

    /* CLI override: direct file should be used */
    write_file("tests/cli_cfg.yml", "Layout:\n  Foo:\n    Enabled: true\n");
    cfg = NULL;
    err = NULL;
    rc = leuko_config_discover_for_file("tests/discover_parent/child/file.rb", "tests/cli_cfg.yml", &cfg, &err);
    assert(rc == 0 && cfg != NULL);
    yaml_node_t *root2 = yaml_document_get_root_node(cfg->doc);
    yaml_node_t *layout2 = yaml_get_mapping_node(cfg->doc, root2, "Layout");
    yaml_node_t *foo = yaml_get_mapping_node(cfg->doc, layout2, "Foo");
    /* sanity: root and Foo present */
    assert(layout2 != NULL && foo != NULL);
    leuko_raw_config_free(cfg);

    return 0;
}
