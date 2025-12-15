#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <glob.h>
#include <stdbool.h>

#include "io/scan.h"

static void set_err(char **err, const char *fmt, ...)
{
    if (!err)
        return;
    va_list ap;
    va_start(ap, fmt);
    char buf[512];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    *err = strdup(buf);
}

/// @brief Append a path to a dynamic array of strings.
/// @param arrp Pointer to the array of strings
/// @param countp Pointer to the count of strings in the array
/// @param path The path string to append
/// @return true on success, false on failure
static bool append_path(char ***arrp, size_t *countp, const char *path)
{
    char *copy = strdup(path);
    if (!copy)
        return false;
    char **tmp = realloc(*arrp, (*countp + 1) * sizeof(char *));
    if (!tmp)
    {
        free(copy);
        return false;
    }
    *arrp = tmp;
    (*arrp)[(*countp)++] = copy;
    return true;
}

/// @brief Return pointer to the extension (including dot) of basename, or NULL.
static const char *get_extension(const char *path)
{
    if (!path)
        return NULL;
    const char *base = strrchr(path, '/');
    if (base)
        base++;
    else
        base = path;
    const char *dot = strrchr(base, '.');
    return dot; /* may be NULL */
}

/// @brief Check if filename looks like a Ruby source file (.rb).
/// @param name The filename to check
/// @return true if it has .rb extension, false otherwise
static bool is_ruby(const char *name)
{
    const char *ext = get_extension(name);
    if (!ext)
        return false;
    return strcmp(ext, ".rb") == 0;
}

/// @brief Recursively scan a directory for Ruby files.
/// @param dirpath The directory path to scan
/// @param out Output array of file paths
/// @param out_count Output count of file paths
/// @param err Output error message on failure
/// @return true on success, false on failure
static bool scan_dir_recursive(const char *dirpath, char ***out, size_t *out_count, char **err)
{
    DIR *d = opendir(dirpath);
    if (!d)
    {
        set_err(err, "opendir('%s'): %s", dirpath, strerror(errno));
        return false;
    }
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL)
    {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;

        char path[PATH_MAX];
        if (snprintf(path, sizeof(path), "%s/%s", dirpath, ent->d_name) >= (int)sizeof(path))
        {
            set_err(err, "path too long");
            closedir(d);
            return false;
        }

        struct stat st;
        if (lstat(path, &st) != 0)
        {
            set_err(err, "stat('%s'): %s", path, strerror(errno));
            closedir(d);
            return false;
        }

        if (S_ISDIR(st.st_mode))
        {
            if (!scan_dir_recursive(path, out, out_count, err))
            {
                closedir(d);
                return false;
            }
        }
        else if (S_ISREG(st.st_mode))
        {
            if (is_ruby(ent->d_name))
            {
                if (append_path(out, out_count, path) != 0)
                {
                    set_err(err, "out of memory");
                    closedir(d);
                    return false;
                }
            }
        }
    }
    closedir(d);
    return true;
}

/// @brief Check if a pattern contains glob metacharacters.
/// @param pattern The pattern string to check
/// @return true if the pattern contains glob metacharacters, false otherwise
bool has_glob_metachar(const char *pattern)
{
    for (const char *p = pattern; *p; ++p)
    {
        if (*p == '*' || *p == '?' || *p == '[')
            return true;
    }
    return false;
}

/// @brief Collect Ruby files from given paths (files, directories, globs).
/// @param paths Input paths array
/// @param paths_count Input paths count
/// @param out_paths Outtut collected paths
/// @param out_count Output collected paths count
/// @param err Output error message on failure
/// @return true on success, false on failure
bool collect_ruby_files(char **paths, size_t paths_count, char ***out_paths, size_t *out_count, char **err)
{
    if (!out_paths || !out_count)
    {
        set_err(err, "invalid arguments");
        return false;
    }

    *out_paths = NULL;
    *out_count = 0;

    for (size_t i = 0; i < paths_count; ++i)
    {
        const char *p = paths[i];
        if (!p)
            continue;

        // '-' is treated as a literal filename token (stdin).
        if (strcmp(p, "-") == 0)
        {
            if (append_path(out_paths, out_count, p) != 0)
            {
                set_err(err, "out of memory");
                return false;
            }
            continue;
        }

        if (has_glob_metachar(p))
        {
            glob_t g;
            memset(&g, 0, sizeof(g));
            int res = glob(p, 0, NULL, &g);
            if (res != 0)
            {
                if (res != GLOB_NOMATCH)
                {
                    // Treat pattern as literal if nothing matched
                    globfree(&g);
                }
                else
                {
                    set_err(err, "glob('%s') failed: %d", p, res);
                    globfree(&g);
                    return false;
                }
            }
            if (g.gl_pathc > 0)
            {
                for (size_t gi = 0; gi < g.gl_pathc; ++gi)
                {
                    const char *mp = g.gl_pathv[gi];
                    struct stat st2;
                    // Symbolic links: follow to determine type
                    if (lstat(mp, &st2) != 0)
                    {
                        set_err(err, "stat('%s'): %s", mp, strerror(errno));
                        globfree(&g);
                        return false;
                    }
                    // Directory: scan recursively
                    if (S_ISDIR(st2.st_mode))
                    {
                        if (!scan_dir_recursive(mp, out_paths, out_count, err))
                        {
                            globfree(&g);
                            return false;
                        }
                    }
                    // Regular file: include if .rb extension
                    else if (S_ISREG(st2.st_mode))
                    {
                        /* Only include regular files that look like Ruby sources (.rb) */
                        if (is_ruby(mp))
                        {
                            if (append_path(out_paths, out_count, mp) != 0)
                            {
                                set_err(err, "out of memory");
                                globfree(&g);
                                return false;
                            }
                        }
                    }
                }
                globfree(&g);
                continue;
            }
            /* fallthrough: no matches -> treat literally */
        }

        struct stat st;
        // Symbolic links: follow to determine type
        if (lstat(p, &st) != 0)
        {
            set_err(err, "stat('%s'): %s", p, strerror(errno));
            return false;
        }
        // Directory: scan recursively
        if (S_ISDIR(st.st_mode))
        {
            if (!scan_dir_recursive(p, out_paths, out_count, err))
                return false;
        }
        // Regular file: include if explicit (regardless of extension)
        else if (S_ISREG(st.st_mode))
        {
            if (append_path(out_paths, out_count, p) != 0)
            {
                set_err(err, "out of memory");
                return false;
            }
        }
        else
        {
            set_err(err, "unsupported file type: %s", p);
            return false;
        }
    }

    return true;
}
