/* tests/test_file_collector.c */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "sources/ruby/leuko_file_collector.h"
#include "utils/allocator/arena.h"

struct collect_ctx
{
    int count;
};

static int collect_cb(const char *path, void *vctx)
{
    struct collect_ctx *c = vctx;
    (void)path;
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

int main(void)
{
    char tmpl[] = "/tmp/leuko_file_collectXXXXXX";
    char *d = mkdtemp(tmpl);
    if (!d)
        return 2;

    char buf[1024];
    snprintf(buf, sizeof(buf), "%s/a.rb", d);
    if (create_file(buf) != 0)
        return 3;
    snprintf(buf, sizeof(buf), "%s/b.rake", d);
    if (create_file(buf) != 0)
        return 3;
    snprintf(buf, sizeof(buf), "%s/Gemfile", d);
    if (create_file(buf) != 0)
        return 3;
    snprintf(buf, sizeof(buf), "%s/other.txt", d);
    if (create_file(buf) != 0)
        return 3;

    snprintf(buf, sizeof(buf), "%s/sub", d);
    if (mkdir(buf, 0755) != 0)
        return 4;
    snprintf(buf, sizeof(buf), "%s/sub/c.rb", d);
    if (create_file(buf) != 0)
        return 3;

    /* Collect starting from directory */
    const char *paths[] = { d };
    struct collect_ctx ctx = {0};
    int r = leuko_collect_ruby_files(paths, 1, collect_cb, &ctx);
    if (r != 0)
        return 5;
    /* a.rb, b.rake, Gemfile, sub/c.rb -> 4 */
    assert(ctx.count == 4);

    /* Collect single file path */
    const char *paths2[] = { "/dev/null", buf /* last created path */ };
    ctx.count = 0;
    r = leuko_collect_ruby_files(paths2, 2, collect_cb, &ctx);
    if (r != 0)
        return 6;
    assert(ctx.count == 1);

    /* Collect into array API */
    char **out = NULL;
    size_t out_count = 0;
    struct leuko_arena *arena = NULL;
    r = leuko_collect_ruby_files_to_array(paths, 1, &out, &out_count, NULL, 0, NULL, 0, 4, &arena);
    if (r != 0)
        return 7;
    /* expect 4 files */
    assert(out_count == 4);
    /* simple sanity: each string is non-null */
    for (size_t i = 0; i < out_count; i++)
    {
        if (!out[i])
        {
            leuko_arena_free(arena);
            return 8;
        }
    }
    /* free arena (which contains strings) */
    leuko_arena_free(arena);

    /* cleanup */
    snprintf(buf, sizeof(buf), "%s/sub/c.rb", d);
    remove(buf);
    snprintf(buf, sizeof(buf), "%s/sub", d);
    rmdir(buf);
    snprintf(buf, sizeof(buf), "%s/a.rb", d);
    remove(buf);
    snprintf(buf, sizeof(buf), "%s/b.rake", d);
    remove(buf);
    snprintf(buf, sizeof(buf), "%s/Gemfile", d);
    remove(buf);
    snprintf(buf, sizeof(buf), "%s/other.txt", d);
    remove(buf);
    rmdir(d);
    return 0;
}
