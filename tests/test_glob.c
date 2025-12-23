/* tests/test_glob.c */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "leuko_glob.h"

/*
 * Simple callback: count matched paths and store last matched path length
 */
struct collect_ctx
{
    int count;
};

static int collect_cb(const char *path, void *vctx)
{
    struct collect_ctx *c = (struct collect_ctx *)vctx;
    c->count++;
    return 0;
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
    snprintf(buf, sizeof(buf), "%s/subdir", root);
    if (mkdir(buf, 0755) != 0)
        return -1;
    snprintf(buf, sizeof(buf), "%s/a.txt", root);
    if (create_file(buf) != 0)
        return -1;
    snprintf(buf, sizeof(buf), "%s/b.log", root);
    if (create_file(buf) != 0)
        return -1;
    snprintf(buf, sizeof(buf), "%s/subdir/c.txt", root);
    if (create_file(buf) != 0)
        return -1;
    return 0;
}

static void cleanup_tree(const char *root)
{
    char buf[1024];
    snprintf(buf, sizeof(buf), "%s/subdir/c.txt", root);
    remove(buf);
    snprintf(buf, sizeof(buf), "%s/subdir", root);
    rmdir(buf);
    snprintf(buf, sizeof(buf), "%s/a.txt", root);
    remove(buf);
    snprintf(buf, sizeof(buf), "%s/b.log", root);
    remove(buf);
}

int main(void)
{
    char tmpl[] = "/tmp/leuko_glob_testXXXXXX";
    char *d = mkdtemp(tmpl);
    if (!d)
        return 2;

    if (make_tree(d) != 0)
        return 2;

    /* Test: include *.txt, exclude subdir/*, non-recursive */
    const char *includes[] = {"*.txt"};
    const char *excludes[] = {"*/subdir/*"};
    leuko_glob_options_t opts = {0};
    opts.root = d;
    opts.includes = includes;
    opts.includes_count = 1;
    opts.excludes = excludes;
    opts.excludes_count = 1;
    opts.recursive = 0;

    struct collect_ctx ctx = {0};
    int r = leuko_glob_walk(&opts, collect_cb, &ctx);
    if (r != 0)
        return 3;
    /* only a.txt should be counted */
    assert(ctx.count == 1);

    /* Test: recursive include *.txt (should find a.txt and subdir/c.txt) */
    opts.includes = includes;
    opts.includes_count = 1;
    opts.excludes_count = 0;
    opts.recursive = 1;

    ctx.count = 0;
    r = leuko_glob_walk(&opts, collect_cb, &ctx);
    if (r != 0)
        return 3;
    assert(ctx.count == 2);

    /* Test: globstar pattern "**/ *.txt " should match across directory levels */
        const char *includes2[] = {"**/*.txt"};
    opts.includes = includes2;
    opts.includes_count = 1;
    opts.excludes_count = 0;
    opts.recursive = 1;

    ctx.count = 0;
    r = leuko_glob_walk(&opts, collect_cb, &ctx);
    if (r != 0)
        return 4;
    assert(ctx.count == 2);

    /* Test: pattern with backslash should be normalized and match (recursive) */
    const char *includes3[] = {"subdir\\*.txt"};
    opts.includes = includes3;
    opts.includes_count = 1;
    opts.excludes_count = 0;
    opts.recursive = 1;

    ctx.count = 0;
    r = leuko_glob_walk(&opts, collect_cb, &ctx);
    if (r != 0)
        return 5;
    assert(ctx.count == 1);

#ifdef _WIN32
    /* Windows-only: matching should be case-insensitive by default */
    snprintf(buf, sizeof(buf), "%s/Case.TXT", root);
    if (create_file(buf) != 0)
        return 6;
    const char *includes4[] = {"case.txt"};
    opts.includes = includes4;
    opts.includes_count = 1;
    opts.excludes_count = 0;
    opts.recursive = 1;

    ctx.count = 0;
    r = leuko_glob_walk(&opts, collect_cb, &ctx);
    if (r != 0)
        return 7;
    assert(ctx.count == 1);
    /* cleanup the extra file */
    snprintf(buf, sizeof(buf), "%s/Case.TXT", root);
    remove(buf);
#endif

    cleanup_tree(d);
    rmdir(d);
    return 0;
}
