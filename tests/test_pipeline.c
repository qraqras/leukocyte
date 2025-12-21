/* Simple pipeline integration test */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "worker/pipeline.h"
#include "configs/generated_config.h"

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

    config_t cfg = {0};
    initialize_config(&cfg);

    char *files[] = {a, b};
    int failures = 0;
    double tp = 0, tb = 0, tv = 0;
    uint64_t th = 0;
    bool ok = leuko_run_pipeline(files, 2, &cfg, 2, true, &failures, &tp, &tb, &tv, &th, 0);
    assert(ok && failures == 0);

    unlink(a);
    unlink(b);
    rmdir(dir);
    free_config(&cfg);
    return 0;
}
