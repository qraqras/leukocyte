#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "configs/walker.h"

static int write_file(const char *path, const char *content)
{
    FILE *f = fopen(path, "w");
    if (!f)
        return -1;
    fprintf(f, "%s", content);
    fclose(f);
    return 0;
}

int main(void)
{
    system("rm -rf tests/tmp_walker && mkdir -p tests/tmp_walker/child/vendor && mkdir -p tests/tmp_walker/child2");

    /* Parent .rubocop.yml excludes vendor/** */
    write_file("tests/tmp_walker/.rubocop.yml", "AllCops:\n  Exclude:\n    - \"vendor/**/*\"\n");

    /* Child has include but should be pruned by parent exclude */
    write_file("tests/tmp_walker/child/.rubocop.yml", "AllCops:\n  Include:\n    - \"**/*.rb\"\n");

    /* Create files */
    write_file("tests/tmp_walker/child/a.rb", "puts 1\n");
    write_file("tests/tmp_walker/child/vendor/b.rb", "puts 2\n");
    write_file("tests/tmp_walker/child2/c.rb", "puts 3\n");

    leuko_collected_file_t *files = NULL;
    size_t count = 0;

    if (leuko_config_walker_collect("tests/tmp_walker", &files, &count) != 0)
    {
        fprintf(stderr, "collect failed\n");
        return 2;
    }

    /* child/vendor/b.rb should be pruned by parent exclude */
    size_t found = 0;
    int has_b = 0;
    for (size_t i = 0; i < count; i++)
    {
        if (strstr(files[i].path, "child/vendor/b.rb"))
            has_b = 1;
        if (strstr(files[i].path, "child/a.rb"))
            found++;
        if (strstr(files[i].path, "child2/c.rb"))
            found++;
    }

    if (has_b)
    {
        fprintf(stderr, "vendor file should be pruned\n");
        leuko_collected_files_free(files, count);
        return 2;
    }

    if (found != 2)
    {
        fprintf(stderr, "unexpected collected count: %zu\n", found);
        leuko_collected_files_free(files, count);
        return 2;
    }

    /* verify each file has a non-null config and that config->dir is a prefix of path */
    for (size_t i = 0; i < count; i++)
    {
        if (!files[i].config)
        {
            fprintf(stderr, "missing config for %s\n", files[i].path);
            leuko_collected_files_free(files, count);
            return 2;
        }
        if (strncmp(files[i].config->dir, files[i].path, strlen(files[i].config->dir)) != 0)
        {
            fprintf(stderr, "config dir not prefix for %s\n", files[i].path);
            leuko_collected_files_free(files, count);
            return 2;
        }
    }

    leuko_collected_files_free(files, count);
    printf("OK\n");
    return 0;
}
