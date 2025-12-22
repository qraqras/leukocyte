/* Simple pipeline integration test */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "worker/pipeline.h"
#include "configs/config.h"

int main(void)
{
    char tmpl[] = "/tmp/leuko_pipeline_XXXXXX";
    char *dir = mkdtemp(tmpl);
    if (!dir)
        return 1;
    char a[512], b[512];
    snprintf(a, sizeof(a), "%s/a.rb", dir);
    snprintf(b, sizeof(b), "%s/b.rb", dir);
    FILE *f = fopen(a, "w");
    if (!f)
        return 1;
    fclose(f);
    f = fopen(b, "w");
    if (!f)
        return 1;
    fclose(f);

    leuko_config_t cfg = {0};
    leuko_config_initialize(&cfg);

    char *files[] = {a, b};
    int failures = 0;
    bool ok = leuko_run_pipeline(files, 2, &cfg, 2, &failures, 0);
    assert(ok && failures == 0);

    unlink(a);
    unlink(b);
    rmdir(dir);
    leuko_config_free(&cfg);
    return 0;
}
