#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "configs/rule_config.h"
#include "configs/conversion/loader.h"
#include "configs/discovery/raw_config.h"

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
    /* parent config defines include and Enabled */
    write_file("tests/tmp_parent.yml", "Layout:\n  IndentationConsistency:\n    Include:\n      - "
                                       "foo"
                                       "\n    Enabled: true\n");

    /* child config defines another include and disables rule */
    write_file("tests/tmp_child.yml", "inherit_from: tests/tmp_parent.yml\nLayout:\n  IndentationConsistency:\n    Include:\n      - "
                                      "bar"
                                      "\n    Enabled: false\n");

    leuko_config_t cfg = {0};
    leuko_config_initialize(&cfg);
    char *err = NULL;
    bool ok = leuko_config_apply_file(&cfg, "tests/tmp_child.yml", &err);
    assert(ok);

    leuko_rule_config_t *r = leuko_rule_config_get_by_index(&cfg, 0);
    assert(r);
    /* Enabled should be child value (false) */
    assert(r->enabled == false);
    /* Include should be concatenated: parent then child */
    assert(r->include_count == 2);
    assert(strcmp(r->include[0], "foo") == 0);
    assert(strcmp(r->include[1], "bar") == 0);

    leuko_config_free(&cfg);
    return 0;
}
