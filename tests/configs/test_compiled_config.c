#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "configs/compiled_config.h"

/* Simple test: create a temp directory with .rubocop.yml and build compiled_config */
int main(void)
{
    const char *td = "tests/tmp_cfg";
    system("rm -rf tests/tmp_cfg && mkdir -p tests/tmp_cfg");
    FILE *f = fopen("tests/tmp_cfg/.rubocop.yml", "w");
    if (!f)
        return 2;
    fprintf(f, "AllCops:\n  Include:\n    - \"**/*.rb\"\n  Exclude:\n    - \"vendor/**/*\"\n");
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

    if (leuko_compiled_config_all_include_count(c) != 1)
    {
        fprintf(stderr, "unexpected include count: %zu\n", leuko_compiled_config_all_include_count(c));
        leuko_compiled_config_unref(c);
        return 2;
    }

    const char *inc = leuko_compiled_config_all_include_at(c, 0);
    if (!inc || strcmp(inc, "**/*.rb") != 0)
    {
        fprintf(stderr, "unexpected include: %s\n", inc ? inc : "(null)");
        leuko_compiled_config_unref(c);
        return 2;
    }

    leuko_compiled_config_unref(c);
    printf("OK\n");
    return 0;
}
