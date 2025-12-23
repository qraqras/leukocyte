#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <fnmatch.h>
#include <uv.h>
#include "sources/leuko_glob.h"
#include "leuko_debug.h"

/*
 * Helpers for pattern matching with globstar support and path normalization.
 * - '**' matches zero or more path segments (including '/').
 * - '*' matches any sequence of characters except '/'.
 * - '?' matches any single character except '/'.
 * Patterns and paths are normalized to use '/' as separator before matching.
 */
#include <ctype.h>

/* Return a newly allocated null-terminated string copying up to len chars. */
static char *leuko_strndup(const char *s, size_t len)
{
    if (!s)
        return NULL;
    char *d = (char *)malloc(len + 1);
    if (!d)
        return NULL;
    memcpy(d, s, len);
    d[len] = '\0';
    return d;
}

/* Normalize path: convert '\\' to '/', collapse consecutive '/' and trim trailing '/'. */
static void leuko_normalize_path(char *out, size_t out_len, const char *in)
{
    if (!out || out_len == 0)
        return;
    size_t oi = 0;
    int last_was_sep = 0;
    for (size_t i = 0; in[i] != '\0' && oi + 1 < out_len; i++)
    {
        char c = in[i];
        /* convert backslash to slash for Windows compatibility */
        if (c == '\\')
            c = '/';
        if (c == '/')
        {
            if (last_was_sep)
                continue; /* collapse */
            last_was_sep = 1;
        }
        else
        {
            last_was_sep = 0;
        }
        out[oi++] = c;
    }
    /* remove trailing slash (but keep root "/") */
    if (oi > 1 && out[oi - 1] == '/')
        oi--;
    out[oi] = '\0';
}

/* Match a single segment pattern against a single segment path using fnmatch.
 * case_insensitive: if non-zero, lowercases both before matching.
 */
static int leuko_segment_matches(const char *segpat, const char *seg, int case_insensitive)
{
    char *pbuf = NULL;
    char *sbuf = NULL;
    int ok = 0;

    if (case_insensitive)
    {
        size_t plen = strlen(segpat);
        size_t slen = strlen(seg);
        pbuf = leuko_strndup(segpat, plen);
        sbuf = leuko_strndup(seg, slen);
        if (!pbuf || !sbuf)
            goto out;
        for (size_t i = 0; pbuf[i]; i++)
            pbuf[i] = (char)tolower((unsigned char)pbuf[i]);
        for (size_t i = 0; sbuf[i]; i++)
            sbuf[i] = (char)tolower((unsigned char)sbuf[i]);
        if (fnmatch(pbuf, sbuf, 0) == 0)
            ok = 1;
    }
    else
    {
        if (fnmatch(segpat, seg, 0) == 0)
            ok = 1;
    }

out:
    if (pbuf)
        free(pbuf);
    if (sbuf)
        free(sbuf);
    return ok;
}

/* Recursive matcher that supports '**' as a segment wildcard. */
static int leuko_pattern_match_internal(const char *pat, const char *path, int case_insensitive)
{
    /* When both are empty, match */
    if (!pat || !path)
        return 0;

    /* If pattern is empty, match only if path is also empty */
    if (*pat == '\0')
        return (*path == '\0');

    const char *p = pat;
    const char *s = path;

    while (*p != '\0')
    {
        /* Handle globstar '**' when it appears as a whole segment or before/after '/'. */
        if (p[0] == '*' && p[1] == '*')
        {
            /* skip consecutive '**' */
            while (p[0] == '*' && p[1] == '*')
                p += 2;
            /* skip single following '/' if present */
            if (*p == '/')
                p++;
            /* If pattern ends with trailing '**', it matches the rest */
            if (*p == '\0')
                return 1;
            /* Try to match the rest of pattern at any segment boundary in s */
            const char *ss = s;
            /* First try matching with zero segments consumed */
            if (leuko_pattern_match_internal(p, ss, case_insensitive))
                return 1;
            while (*ss != '\0')
            {
                /* advance to next segment start */
                const char *next = strchr(ss, '/');
                if (!next)
                    break;
                ss = next + 1;
                if (leuko_pattern_match_internal(p, ss, case_insensitive))
                    return 1;
            }
            return 0;
        }

        /* Extract next segment from pattern (up to '/' or end) */
        const char *pseg_end = strchr(p, '/');
        size_t plen = pseg_end ? (size_t)(pseg_end - p) : strlen(p);
        char *pseg = leuko_strndup(p, plen);
        if (!pseg)
            return 0;

        /* Extract next segment from path */
        const char *sseg_end = strchr(s, '/');
        size_t slen = sseg_end ? (size_t)(sseg_end - s) : strlen(s);
        char *sseg = leuko_strndup(s, slen);
        if (!sseg)
        {
            free(pseg);
            return 0;
        }

        /* If the path has no more segments but pattern expects one, fail */
        if (slen == 0 && plen > 0)
        {
            free(pseg);
            free(sseg);
            return 0;
        }

        /* match the two segments */
        int seg_ok = leuko_segment_matches(pseg, sseg, case_insensitive);
        free(pseg);
        free(sseg);
        if (!seg_ok)
            return 0;

        /* advance p and s past this segment and optional '/' */
        if (pseg_end)
            p = pseg_end + 1;
        else
            p += plen;
        if (sseg_end)
            s = sseg_end + 1;
        else
            s += slen;
    }

    /* Pattern exhausted: match only if path also exhausted */
    return (*s == '\0');
}

/* Public wrapper: normalize pattern and path and match.
 * Case-insensitive matching is enabled by default on Windows.
 */
static int leuko_pattern_match(const char *pattern, const char *path)
{
    if (!pattern || !path)
        return 0;
    char pbuf[4096];
    char sbuf[4096];
    leuko_normalize_path(pbuf, sizeof(pbuf), pattern);
    leuko_normalize_path(sbuf, sizeof(sbuf), path);

#ifdef _WIN32
    int case_insensitive = 1;
#else
    int case_insensitive = 0;
#endif

    return leuko_pattern_match_internal(pbuf, sbuf, case_insensitive);
}

/* Return 1 if `name` matches any of the patterns.
 * If pattern_count == 0, treat as "match all".
 */
static int leuko_path_matches_any(const char *name, const char **patterns, size_t pattern_count)
{
    if (!patterns || pattern_count == 0)
        return 1; /* no patterns means match all */

    for (size_t i = 0; i < pattern_count; i++)
    {
        const char *p = patterns[i];
        if (!p)
            continue;
        if (leuko_pattern_match(p, name))
            return 1;
    }
    return 0;
}

/*
 * Build a full path by joining dir and name into outbuf (size outbuf_len).
 * Returns 0 on success, negative on error.
 */
static int leuko_join_path(char *outbuf, size_t outbuf_len, const char *dir, const char *name)
{
    if (!dir || dir[0] == '\0')
    {
        if (strlen(name) + 1 > outbuf_len)
            return -1;
        strcpy(outbuf, name);
        return 0;
    }

    size_t dlen = strlen(dir);
    int need_sep = (dir[dlen - 1] != '/');
    size_t needed = dlen + (need_sep ? 1 : 0) + strlen(name) + 1;
    if (needed > outbuf_len)
        return -1;
    strcpy(outbuf, dir);
    if (need_sep)
        strcat(outbuf, "/");
    strcat(outbuf, name);
    return 0;
}

/**
 * @brief Internal helper: walk directory using libuv loop.
 * @param loop Pointer to initialized uv_loop_t.
 * @param root Directory path to walk.
 * @param opts Pointer to leuko_glob_options_t with options.
 * @param cb Callback function invoked with full path of matched files.
 *           Return true to continue, false to abort the walk early.
 * @param ctx User context pointer passed to callback.
 * @return 0 on success, non-zero on error (or callback return value).
 */
static int leuko_walk_dir_uv_loop(uv_loop_t *loop, const char *root, const leuko_glob_options_t *opts, bool (*cb)(const char *path, void *ctx), void *ctx)
{
    uv_fs_t req;
    int r = uv_fs_scandir(loop, &req, root, 0, NULL);
    if (r < 0)
    {
        uv_fs_req_cleanup(&req);
        return -1;
    }

    uv_dirent_t dent;
    char pathbuf[4096];

    while (uv_fs_scandir_next(&req, &dent) == 0)
    {
        const char *name = dent.name;
        /* skip . and .. */
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
            continue;

        if (leuko_join_path(pathbuf, sizeof(pathbuf), root, name) != 0)
        {
            uv_fs_req_cleanup(&req);
            return -2;
        }

        /* Exclude match takes precedence */
        if (leuko_path_matches_any(pathbuf, (const char **)opts->excludes, opts->excludes_count))
            continue;

        /* If include patterns are present and this path doesn't match any, skip */
        if (opts->includes_count > 0 && !leuko_path_matches_any(pathbuf, (const char **)opts->includes, opts->includes_count))
            ; /* skip */
        else
        {
            if (dent.type == UV_DIRENT_FILE || dent.type == UV_DIRENT_LINK)
            {
                int cr = cb(pathbuf, ctx);
                if (cr != 0)
                {
                    uv_fs_req_cleanup(&req);
                    return cr;
                }
            }
        }

        if (opts->recursive && dent.type == UV_DIRENT_DIR)
        {
            int cr = leuko_walk_dir_uv_loop(loop, pathbuf, opts, cb, ctx);
            if (cr != 0)
            {
                uv_fs_req_cleanup(&req);
                return cr;
            }
        }
    }

    uv_fs_req_cleanup(&req);
    return 0;
}

/**
 * @brief Walk directories starting from opts->root (or current dir if NULL),
 *        applying include/exclude patterns and invoking callback for matched files.
 * @param opts Pointer to leuko_glob_options_t with options.
 * @param cb Callback function invoked with full path of matched files.
 *           Return true to continue, false to abort the walk early.
 * @param ctx User context pointer passed to callback.
 * @return true on success, false on error.
 */
bool leuko_glob_walk(const leuko_glob_options_t *opts, bool (*cb)(const char *path, void *ctx), void *ctx)
{
    if (!opts || !cb)
        return false;
    const char *root = opts->root ? opts->root : ".";
    uv_loop_t loop;
    if (uv_loop_init(&loop) != 0)
    {
        return false;
    }
    int r = leuko_walk_dir_uv_loop(&loop, root, opts, cb, ctx);
    uv_loop_close(&loop);
    return (r == 0);
}

/* ---------------- Parallel async walker ---------------- */

/* NOTE: This parallel walker uses libuv async scandir completions on the
 * provided loop. The callbacks are executed on the same loop thread, so we
 * can update counters and call the user callback directly without extra
 * synchronization. We maintain a queue of directories waiting to be scanned to
 * enforce a max concurrency limit.
 */

typedef struct leuko_par_ctx_s
{
    uv_loop_t *loop;
    size_t max_concurrency;
    size_t active_scans;
    size_t outstanding; /* number of outstanding requests */

    /* queue of pending directory paths */
    char **queue;
    size_t queue_count;
    size_t queue_cap;

    /* options copy (shallow copy of pointers is OK) */
    leuko_glob_options_t opts_template;

    bool (*user_cb)(const char *path, void *ctx);
    void *user_ctx;

    int error;
    bool abort_flag;
} leuko_par_ctx_t;

static int leuko_par_enqueue_dir(leuko_par_ctx_t *c, const char *path)
{
    if (!c || !path)
        return -1;
    if (c->queue_count >= c->queue_cap)
    {
        size_t nc = c->queue_cap ? c->queue_cap * 2 : 16;
        char **tmp = realloc(c->queue, nc * sizeof(char *));
        if (!tmp)
            return -2;
        c->queue = tmp;
        c->queue_cap = nc;
    }
    char *dup = strdup(path);
    if (!dup)
        return -3;
    c->queue[c->queue_count++] = dup;
    return 0;
}

static char *leuko_par_dequeue_dir(leuko_par_ctx_t *c)
{
    if (!c || c->queue_count == 0)
        return NULL;
    char *p = c->queue[--c->queue_count];
    return p;
}

/* forward */
static void leuko_par_start_scan(leuko_par_ctx_t *c, const char *path);

static void leuko_par_scandir_cb(uv_fs_t *req)
{
    leuko_par_ctx_t *c = (leuko_par_ctx_t *)req->data;
    char *req_path = NULL;
    if (req && req->path)
        req_path = (char *)req->path; /* uv stores path internally on some platforms */

    if (!c)
    {
        if (req)
        {
            uv_fs_req_cleanup(req);
            free(req);
        }
        return;
    }

    if (req->result < 0)
    {
        LDEBUG("uv_fs_scandir failure on '%s': %s", req_path ? req_path : "(unknown)", uv_strerror((int)req->result));
        c->error = (int)req->result;
    }
    else
    {
        uv_dirent_t dent;
        char pathbuf[4096];
        while (uv_fs_scandir_next(req, &dent) == 0)
        {
            const char *name = dent.name;
            if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
                continue;
            if (leuko_join_path(pathbuf, sizeof(pathbuf), req_path ? req_path : ".", name) != 0)
            {
                c->error = -1;
                break;
            }

            /* Exclude match takes precedence */
            if (c->opts_template.excludes && c->opts_template.excludes_count > 0 && leuko_path_matches_any(pathbuf, (const char **)c->opts_template.excludes, c->opts_template.excludes_count))
                continue;

            /* If include patterns are present and this path doesn't match any, skip */
            if (c->opts_template.includes_count > 0 && !leuko_path_matches_any(pathbuf, (const char **)c->opts_template.includes, c->opts_template.includes_count))
                ; /* skip */
            else
            {
                if (dent.type == UV_DIRENT_FILE || dent.type == UV_DIRENT_LINK)
                {
                    if (c->user_cb)
                    {
                        bool keep = c->user_cb(pathbuf, c->user_ctx);
                        if (!keep)
                        {
                            c->abort_flag = true;
                            break;
                        }
                    }
                }
                else if (dent.type == UV_DIRENT_DIR)
                {
                    /* schedule or enqueue */
                    if (c->active_scans < c->max_concurrency)
                    {
                        leuko_par_start_scan(c, pathbuf);
                    }
                    else
                    {
                        leuko_par_enqueue_dir(c, pathbuf);
                    }
                }
            }
            if (c->abort_flag)
                break;
        }
    }

    /* cleanup request */
    uv_fs_req_cleanup(req);
    if (req)
        free(req);

    /* decrement counts */
    if (c->active_scans > 0)
        c->active_scans--;
    if (c->outstanding > 0)
        c->outstanding--;

    /* if aborted, clear pending queue */
    if (c->abort_flag)
    {
        /* free queued paths */
        for (size_t i = 0; i < c->queue_count; i++)
            free(c->queue[i]);
        c->queue_count = 0;
        uv_stop(c->loop);
        return;
    }

    /* start queued scans while we have capacity */
    while (c->active_scans < c->max_concurrency && c->queue_count > 0)
    {
        char *p = leuko_par_dequeue_dir(c);
        if (p)
        {
            leuko_par_start_scan(c, p);
            free(p);
        }
    }

    /* finished? stop loop */
    if (c->outstanding == 0)
    {
        uv_stop(c->loop);
        return;
    }
}

static void leuko_par_start_scan(leuko_par_ctx_t *c, const char *path)
{
    if (!c || !path)
        return;
    uv_fs_t *req = malloc(sizeof(uv_fs_t));
    if (!req)
    {
        c->error = -1;
        return;
    }
    memset(req, 0, sizeof(*req));
    req->data = c;

    int r = uv_fs_scandir(c->loop, req, path, 0, leuko_par_scandir_cb);
    if (r < 0)
    {
        c->error = r;
        uv_fs_req_cleanup(req);
        free(req);
        return;
    }
    c->active_scans++;
    c->outstanding++;
}

bool leuko_glob_walk_parallel(const leuko_glob_options_t *opts, bool (*cb)(const char *path, void *ctx), void *ctx, size_t max_concurrency)
{
    if (!opts || !cb || max_concurrency == 0)
        return false;

    uv_loop_t loop;
    if (uv_loop_init(&loop) != 0)
    {
        return false;
    }

    leuko_par_ctx_t c;
    memset(&c, 0, sizeof(c));
    c.loop = &loop;
    c.max_concurrency = max_concurrency;
    c.active_scans = 0;
    c.outstanding = 0;
    c.queue = NULL;
    c.queue_count = 0;
    c.queue_cap = 0;
    c.opts_template = *opts; /* shallow copy of pointers/counts */
    c.user_cb = cb;
    c.user_ctx = ctx;
    c.error = 0;
    c.abort_flag = false;

    /* kick off with root */
    const char *root = opts->root ? opts->root : ".";
    leuko_par_start_scan(&c, root);

    /* run loop until all finish */
    uv_run(&loop, UV_RUN_DEFAULT);
    uv_loop_close(&loop);

    /* free any remaining queued paths */
    for (size_t i = 0; i < c.queue_count; i++)
        free(c.queue[i]);
    free(c.queue);

    return (c.error == 0 && !c.abort_flag);
}
