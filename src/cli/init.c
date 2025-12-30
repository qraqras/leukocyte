#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include "cli/init.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static int write_file_atomic(const char *path, const char *content, mode_t mode)
{
    char tmp[PATH_MAX];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    FILE *f = fopen(tmp, "w");
    if (!f)
        return -1;
    if (fputs(content, f) == EOF)
    {
        fclose(f);
        unlink(tmp);
        return -1;
    }
    fclose(f);
    if (rename(tmp, path) != 0)
    {
        unlink(tmp);
        return -1;
    }
    chmod(path, mode);
    return 0;
}

int leuko_cli_init(const char *project_dir, bool apply_gitignore)
{
    char cwd[PATH_MAX];
    if (project_dir)
    {
        if (strlen(project_dir) >= sizeof(cwd))
            return LEUKO_EXIT_INVALID;
        strcpy(cwd, project_dir);
    }
    else
    {
        if (!getcwd(cwd, sizeof(cwd)))
        {
            perror("getcwd");
            return LEUKO_EXIT_INVALID;
        }
    }

    char base[PATH_MAX];
    snprintf(base, sizeof(base), "%s/.leukocyte", cwd);

    /* create base dir if not exists */
    if (mkdir(base, 0700) != 0)
    {
        if (errno != EEXIST)
        {
            perror("mkdir");
            return LEUKO_EXIT_INVALID;
        }
    }

    /* README */
    char readme_path[PATH_MAX];
    snprintf(readme_path, sizeof(readme_path), "%s/README", base);
    const char *readme = "# .leukocyte\n\nThis directory stores generated Leukocyte artifacts (resolved RuboCop configs in JSON form).\n\nTo generate configs, run:\n\n  leuko --sync\n\nBy default the repository's .gitignore should exclude generated files under .leukocyte/configs/.\n";
    write_file_atomic(readme_path, readme, 0644);

    /* gitignore.template */
    char gi_path[PATH_MAX];
    snprintf(gi_path, sizeof(gi_path), "%s/gitignore.template", base);
    const char *gi = "# Ignore generated Leukocyte artifacts\nleuko*.lock\n.leukocyte/configs/\n.leukocyte/index.json\n.leukocyte/*.tmp\n";
    write_file_atomic(gi_path, gi, 0644);

    /* configs/ is created by `leuko --sync` when needed; do not create it here to keep init non-destructive */

    /* Optionally apply gitignore to project root */
    if (apply_gitignore)
    {
        char gitignore_path[PATH_MAX];
        snprintf(gitignore_path, sizeof(gitignore_path), "%s/.gitignore", cwd);
        FILE *f = fopen(gitignore_path, "a");
        if (f)
        {
            fputs("\n# Ignore generated Leukocyte artifacts\n.leukocyte/configs/\n.leukocyte/index.json\n.leukocyte/*.tmp\n", f);
            fclose(f);
        }
        else
        {
            /* Non-fatal: warn and continue */
            fprintf(stderr, "Warning: could not append to %s\n", gitignore_path);
        }
    }

    printf("Initialized %s\n", base);
    return LEUKO_EXIT_OK;
}
