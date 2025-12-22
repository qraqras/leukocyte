/* Test warm cache behaviour */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "configs/discover.h"
#include "configs/generated_config.h"

int main(void)
{
    char tmpl[] = "/tmp/leuko_warm_XXXXXX";
    char *dir = mkdtemp(tmpl);
    if (!dir)
        return 1;

    /* write config in dir */
    char cfgpath[512];
    snprintf(cfgpath, sizeof(cfgpath), "%s/.rubocop.yml", dir);
    FILE *f = fopen(cfgpath, "w");
    if (!f)
        return 1;
    fprintf(f, "Style/Example:\n  Enabled: true\n");
    fclose(f);

    /* create two files in same dir */
    char a[512], b[512];
    snprintf(a, sizeof(a), "%s/a.rb", dir);
    snprintf(b, sizeof(b), "%s/b.rb", dir);
    f = fopen(a, "w");
    if (!f)
        return 1;
    fclose(f);
    f = fopen(b, "w");
    if (!f)
        return 1;
    fclose(f);

    /* warm cache in parallel */
    char *files[] = {a, b};
    char *err = NULL;
    int rc = leuko_config_warm_cache_for_files(files, 2, 2, &err);
    if (rc != 0)
    {
        if (err)
            fprintf(stderr, "warm failed: %s\n", err);
        else
            fprintf(stderr, "warm failed: unknown\n");
        return 1;
    }
    printf("warm succeeded\n");

    /* both should return same cached config pointer */
    const config_t *c1 = NULL;
    const config_t *c2 = NULL;
    /* Debug: call discover directly to see what happens */
    leuko_raw_config_t *raw = NULL;
    char *derr = NULL;
    int drc = leuko_config_discover_for_file(a, NULL, &raw, &derr);
    printf("discover returned %d raw=%p derr=%p\n", drc, (void *)raw, (void *)derr);
    if (drc != 0)
    {
        if (derr)
            fprintf(stderr, "discover err: %s\n", derr);
        return 1;
    }
    if (raw)
    {
        printf("discovered path: %s\n", raw->path);
        leuko_raw_config_free(raw);
    }

    rc = leuko_config_get_cached_config_for_file(a, &c1, &err);
    printf("get_cached for a returned %d err=%p\n", rc, (void *)err);
    if (rc != 0)
    {
        if (err)
            fprintf(stderr, "get_cached failed for a: %s\n", err);
        return 1;
    }
    rc = leuko_config_get_cached_config_for_file(b, &c2, &err);
    printf("get_cached for b returned %d err=%p\n", rc, (void *)err);
    if (rc != 0)
    {
        if (err)
            fprintf(stderr, "get_cached failed for b: %s\n", err);
        return 1;
    }

    printf("cfg pointers: %p %p\n", (void *)c1, (void *)c2);

    /* both pointers should be non-null and equal (same cache entry for directory) */
    assert(c1 != NULL && c2 != NULL && c1 == c2);
    printf("assert ok\n");

    /* cleanup */
    unlink(a);
    unlink(b);
    unlink(cfgpath);
    rmdir(dir);
    leuko_config_clear_cache();
    return 0;
}
