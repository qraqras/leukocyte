#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "configs/compiled_config.h"
#include "configs/walker.h"

int main(void)
{
    const char *td = "tests/tmp_precompile";
    system("rm -rf tests/tmp_precompile && mkdir -p tests/tmp_precompile/vendor && mkdir -p tests/tmp_precompile/lib && touch tests/tmp_precompile/a.rb tests/tmp_precompile/vendor/b.rb tests/tmp_precompile/lib/c.txt");
    FILE *f = fopen("tests/tmp_precompile/.rubocop.yml", "w");
    if (!f)
        return 2;
    // include all rb, but exclude vendor/**/*
    fprintf(f, "AllCops:\n  Include:\n    - \"**/*.rb\"\n  Exclude:\n    - \"vendor/**/*\"\n");
    fclose(f);

    /* Build compiled config and verify precompiled regex counts */
    leuko_compiled_config_t *c = leuko_compiled_config_build(td, NULL);
    if (!c)
    {
        fprintf(stderr, "build failed\n");
        return 2;
    }

    const leuko_all_cops_config_t *ac = leuko_compiled_config_all_cops(c);
    if (!ac)
    {
        fprintf(stderr, "no all_cops\n");
        leuko_compiled_config_unref(c);
        return 2;
    }

    if (ac->include_re_count == 0 || ac->exclude_re_count == 0)
    {
        fprintf(stderr, "regex not compiled: include_re_count=%zu exclude_re_count=%zu\n", ac->include_re_count, ac->exclude_re_count);
        leuko_compiled_config_unref(c);
        return 2;
    }

    /* Run walker and ensure vendor/b.rb is excluded and a.rb is included */
    leuko_collected_file_t *files = NULL;
    size_t count = 0;
    int rc = leuko_config_walker_collect(td, &files, &count);
    if (rc != 0)
    {
        fprintf(stderr, "walker failed\n");
        leuko_compiled_config_unref(c);
        return 2;
    }

    int saw_a = 0;
    int saw_b = 0;
    for (size_t i = 0; i < count; i++)
    {
        const char *p = files[i].path;
        if (strstr(p, "a.rb"))
            saw_a = 1;
        if (strstr(p, "vendor/b.rb"))
            saw_b = 1;
    }

    leuko_collected_files_free(files, count);
    leuko_compiled_config_unref(c);

    if (!saw_a)
    {
        fprintf(stderr, "expected a.rb included\n");
        return 2;
    }
    if (saw_b)
    {
        fprintf(stderr, "vendor/b.rb should be excluded\n");
        return 2;
    }

    printf("OK\n");
    return 0;
}
