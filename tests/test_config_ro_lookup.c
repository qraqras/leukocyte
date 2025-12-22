/* Test read-only cache lookup behavior for worker threads */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "configs/discover.h"
#include "configs/generated_config.h"

int main(void)
{
    char tmpl[] = "/tmp/leuko_ro_XXXXXX";
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

    /* file in same dir */
    char a[512];
    snprintf(a, sizeof(a), "%s/a.rb", dir);
    f = fopen(a, "w");
    if (!f)
        return 1;
    fclose(f);

    /* Ensure cache is clear */
    leuko_config_clear_cache();

    /* RO lookup before warm: should return NULL and 0 */
    const config_t *c = NULL;
    int rc = leuko_config_get_cached_config_for_file_ro(a, &c);
    if (rc != 0)
    {
        fprintf(stderr, "ro lookup failed (rc=%d)\n", rc);
        return 1;
    }
    if (c != NULL)
    {
        fprintf(stderr, "ro lookup returned a config before warm\n");
        return 1;
    }

    /* Warm cache for file */
    char *files[] = {a};
    char *err = NULL;
    rc = leuko_config_warm_cache_for_files(files, 1, 1, &err);
    if (rc != 0)
    {
        if (err)
            fprintf(stderr, "warm failed: %s\n", err);
        else
            fprintf(stderr, "warm failed: unknown\n");
        return 1;
    }

    /* RO lookup after warm should return a config pointer */
    c = NULL;
    rc = leuko_config_get_cached_config_for_file_ro(a, &c);
    if (rc != 0 || c == NULL)
    {
        fprintf(stderr, "ro lookup after warm did not return config (rc=%d c=%p)\n", rc, (void *)c);
        return 1;
    }
    const void *prev_cfg_ptr = (const void *)c;

    /* Additional invalidation semantics (mtime change) are tested elsewhere; */
    /* For workers we only guarantee that RO lookup returns NULL before warm and non-NULL after warm. */

    /* cleanup */
    unlink(a);
    unlink(cfgpath);
    rmdir(dir);
    leuko_config_clear_cache();
    return 0;
}
