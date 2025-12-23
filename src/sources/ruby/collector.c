#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

#include "sources/ruby/collector.h"
#include "sources/leuko_glob.h"
#include "leuko_debug.h"
#include "utils/allocator/arena.h"

/* push state used by array collector */
struct push_state
{
    char ***arrp;
    size_t *countp;
    size_t *capp;
    struct leuko_arena *arena;
};

static bool push_path_cb(const char *path, void *vstate)
{
    struct push_state *s = (struct push_state *)vstate;
    char *dup = leuko_arena_strdup(s->arena, path);
    if (!dup)
        return false;
    if (*(s->countp) >= *(s->capp))
    {
        size_t nc = (*(s->capp)) * 2;
        char **tmp = realloc(*(s->arrp), nc * sizeof(char *));
        if (!tmp)
            return false;
        *(s->arrp) = tmp;
        *(s->capp) = nc;
    }
    (*(s->arrp))[(*(s->countp))++] = dup;
    return true;
}

/* Default patterns to collect Ruby related files */
static const char *leuko_ruby_default_patterns[] = {
    "**/*.rb",
    "**/*.rake",
    "Rakefile",
    "Gemfile",
    "config.ru",
    "Capfile",
    "Thorfile",
};
static const size_t leuko_ruby_default_patterns_count = sizeof(leuko_ruby_default_patterns) / sizeof(leuko_ruby_default_patterns[0]);

/* Wrapper to adapt user int-returning callback to leuko_glob_walk's callback
 * which expects a return interpreted as "0 continue, non-zero abort".
 */
struct leuko_cb_wrap_s
{
    int (*user_cb)(const char *path, void *ctx);
    void *user_ctx;
    int last_ret; /* store last non-zero return from user cb */
};

static bool leuko_glob_cb_adapter(const char *path, void *vctx)
{
    struct leuko_cb_wrap_s *w = (struct leuko_cb_wrap_s *)vctx;
    int r = w->user_cb(path, w->user_ctx);
    if (r != 0)
    {
        w->last_ret = r;
        return false; /* signal to stop walking */
    }
    return true; /* continue */
}

/*
 * Public API: collect Ruby files from the given paths array.
 * If a path is a file and matches Ruby patterns, invoke cb(path, ctx).
 * If a path is a directory, recursively walk it and invoke cb for matches.
 */
int leuko_collect_ruby_files(const char **paths, size_t paths_count, int (*cb)(const char *path, void *ctx), void *ctx)
{
    if (!paths || paths_count == 0 || !cb)
        return -1;

    struct stat st;
    struct leuko_cb_wrap_s wrap = {.user_cb = cb, .user_ctx = ctx, .last_ret = 0};

    for (size_t i = 0; i < paths_count; i++)
    {
        const char *p = paths[i];
        if (!p)
            continue;

        if (stat(p, &st) != 0)
        {
            /* If path does not exist, skip it */
            LDEBUG("leuko_collect_ruby_files: stat('%s') failed: %s", p, strerror(errno));
            continue;
        }

        if (S_ISREG(st.st_mode))
        {
            /* file: simple filename checks for common Ruby-related files (avoid
             * depending on internal matcher symbols).
             */
            const char *base = strrchr(p, '/');
            if (!base)
                base = p;
            else
                base++; /* skip '/' */

            size_t baselen = strlen(base);
            int matched = 0;
            /* exact filenames */
            const char *exacts[] = {"Rakefile", "Gemfile", "config.ru", "Capfile", "Thorfile"};
            for (size_t ei = 0; ei < sizeof(exacts) / sizeof(exacts[0]); ei++)
            {
                if (strcmp(base, exacts[ei]) == 0)
                {
                    matched = 1;
                    break;
                }
            }
            /* extensions */
            if (!matched)
            {
                if (baselen > 3 && strcmp(base + baselen - 3, ".rb") == 0)
                    matched = 1;
                else if (baselen > 5 && strcmp(base + baselen - 5, ".rake") == 0)
                    matched = 1;
            }

            if (matched)
            {
                int r = cb(p, ctx);
                if (r != 0)
                    return r;
            }
            continue;
        }
        else if (S_ISDIR(st.st_mode))
        {
            leuko_glob_options_t opts = {0};
            opts.root = p;
            opts.includes = leuko_ruby_default_patterns;
            opts.includes_count = leuko_ruby_default_patterns_count;
            opts.excludes = NULL;
            opts.excludes_count = 0;
            opts.recursive = 1;

            bool walk_ok = leuko_glob_walk(&opts, leuko_glob_cb_adapter, &wrap);
            if (!walk_ok)
            {
                /* if adapter set last_ret (user aborted), return it */
                if (wrap.last_ret != 0)
                    return wrap.last_ret;
                return -2; /* walk error */
            }
            if (wrap.last_ret != 0)
                return wrap.last_ret;
        }
        else
        {
            /* ignore other types (symlink handling could be added) */
            continue;
        }
    }

    return 0;
}

/* Collect into an array (arena-backed) implementation */
#include "utils/allocator/arena.h"

int leuko_collect_ruby_files_to_array(const char **paths,
                                      size_t paths_count,
                                      char ***out_paths,
                                      size_t *out_count,
                                      const char **includes,
                                      size_t includes_count,
                                      const char **excludes,
                                      size_t excludes_count,
                                      size_t initial_capacity,
                                      struct leuko_arena **out_arena)
{
    if (!paths || paths_count == 0 || !out_paths || !out_count || !out_arena)
        return -1;

    size_t cap = initial_capacity && initial_capacity > 0 ? initial_capacity : 16;
    char **arr = malloc(cap * sizeof(char *));
    if (!arr)
        return -2;
    size_t count = 0;

    size_t arena_hint = (initial_capacity && initial_capacity > 0) ? (initial_capacity * 256) : 4096;
    struct leuko_arena *arena = leuko_arena_new(arena_hint);
    if (!arena)
    {
        free(arr);
        return -3;
    }

    /* wrapper state */
    struct push_state
    {
        char ***arrp;
        size_t *countp;
        size_t *capp;
        struct leuko_arena *arena;
    } st;
    st.arrp = &arr;
    st.countp = &count;
    st.capp = &cap;
    st.arena = arena;

    /* process paths */
    struct stat stbuf;
    for (size_t i = 0; i < paths_count; i++)
    {
        const char *p = paths[i];
        if (!p)
            continue;
        if (stat(p, &stbuf) != 0)
        {
            LDEBUG("leuko_collect_ruby_files_to_array: stat('%s') failed: %s", p, strerror(errno));
            continue;
        }
        if (S_ISREG(stbuf.st_mode))
        {
            int matched = 0;
            const char *base = strrchr(p, '/');
            if (!base)
                base = p;
            else
                base++;
            size_t baselen = strlen(base);
            const char *exacts[] = {"Rakefile", "Gemfile", "config.ru", "Capfile", "Thorfile"};
            for (size_t ei = 0; ei < sizeof(exacts) / sizeof(exacts[0]); ei++)
                if (strcmp(base, exacts[ei]) == 0)
                {
                    matched = 1;
                    break;
                }
            if (!matched)
            {
                if (baselen > 3 && strcmp(base + baselen - 3, ".rb") == 0)
                    matched = 1;
                else if (baselen > 5 && strcmp(base + baselen - 5, ".rake") == 0)
                    matched = 1;
            }
            if (includes && includes_count > 0)
            {
                if (!leuko_path_matches_any(p, includes, includes_count))
                    matched = 0;
            }
            if (excludes && excludes_count > 0)
            {
                if (leuko_path_matches_any(p, excludes, excludes_count))
                    matched = 0;
            }
            if (matched)
            {
                if (!push_path_cb(p, &st))
                {
                    leuko_arena_free(arena);
                    free(arr);
                    return -4;
                }
            }
            continue;
        }
        else if (S_ISDIR(stbuf.st_mode))
        {
            leuko_glob_options_t opts = {0};
            opts.root = p;
            if (includes && includes_count > 0)
            {
                opts.includes = includes;
                opts.includes_count = includes_count;
            }
            else
            {
                opts.includes = leuko_ruby_default_patterns;
                opts.includes_count = leuko_ruby_default_patterns_count;
            }
            if (excludes && excludes_count > 0)
            {
                opts.excludes = excludes;
                opts.excludes_count = excludes_count;
            }
            opts.recursive = 1;

            bool walk_ok = leuko_glob_walk(&opts, push_path_cb, &st);
            if (!walk_ok)
            {
                leuko_arena_free(arena);
                free(arr);
                return -5;
            }
            continue;
        }
    }

    *out_paths = arr;
    *out_count = count;
    *out_arena = arena;
    return 0;
}
