/* tests/test_glob_parallel.c */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "leuko_glob.h"

struct collect_ctx
{
    int count;
};

static bool collect_cb(const char *path, void *vctx)
{
    struct collect_ctx *c = vctx;
    (void)path;
    c->count++;
    return true; /* continue */
}

static int create_file(const char *path)
{
    FILE *f = fopen(path, "w");
    if (!f)
        return -1;
    fputs("x\n", f);
    fclose(f);
    return 0;
}

static int make_tree(const char *root)
{
    char buf[1024];
    for (int d = 0; d < 4; d++)
    {
        snprintf(buf, sizeof(buf), "%s/dir%d", root, d);
        if (mkdir(buf, 0755) != 0)
            return -1;
        for (int f = 0; f < 10; f++)
        {
            snprintf(buf, sizeof(buf), "%s/dir%d/file%d.txt", root, d, f);
            if (create_file(buf) != 0)
                return -1;
        }
    }
    return 0;
}

static void cleanup_tree(const char *root)
{
    char buf[1024];
    for (int d = 0; d < 4; d++)
    {
        for (int f = 0; f < 10; f++)
        {
            snprintf(buf, sizeof(buf), "%s/dir%d/file%d.txt", root, d, f);
            remove(buf);
        }
        snprintf(buf, sizeof(buf), "%s/dir%d", root, d);
        rmdir(buf);
    }
}

int main(void)
{
    char tmpl[] = "/tmp/leuko_glob_parallelXXXXXX";
    char *d = mkdtemp(tmpl);
    if (!d)
        return 2;

    if (make_tree(d) != 0)
        return 3;

    leuko_glob_options_t opts = {0};
    opts.root = d;
    const char *inc[] = {"**/*.txt"};
    opts.includes = inc;
    opts.includes_count = 1;
    opts.recursive = 1;

    struct collect_ctx ctx = {0};
    bool ok = leuko_glob_walk_parallel(&opts, collect_cb, &ctx, 4);
    if (!ok)
        return 4;
    /* 4 dirs * 10 files each = 40 */
    assert(ctx.count == 40);

    cleanup_tree(d);
    rmdir(d);
    return 0;
}
