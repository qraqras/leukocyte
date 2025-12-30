#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include "cli/sync.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

int leuko_cli_sync(const char *project_dir, const char *script_path, const char *outdir, const char *index_path)
{
    const char *script = script_path ? script_path : "scripts/sync_configs.rb";

    char cwd_buf[PATH_MAX];
    if (project_dir)
    {
        if (strlen(project_dir) >= sizeof(cwd_buf))
        {
            fprintf(stderr, "Project dir path too long\n");
            return LEUKO_EXIT_INVALID;
        }
        strncpy(cwd_buf, project_dir, sizeof(cwd_buf) - 1);
        cwd_buf[sizeof(cwd_buf) - 1] = '\0';
    }
    else
    {
        if (!getcwd(cwd_buf, sizeof(cwd_buf)))
        {
            perror("getcwd");
            return LEUKO_EXIT_INVALID;
        }
    }

    /* Ensure .leukocyte exists (init was run) */
    {
        char base_dir[PATH_MAX];
        int m = snprintf(base_dir, sizeof(base_dir), "%s/.leukocyte", cwd_buf);
        if (m < 0 || (size_t)m >= sizeof(base_dir))
        {
            fprintf(stderr, "Internal path construction failed\n");
            return LEUKO_EXIT_INVALID;
        }
        struct stat st;
        if (stat(base_dir, &st) != 0 || !S_ISDIR(st.st_mode))
        {
            fprintf(stderr, ".leukocyte not initialized in %s; run 'leuko --init' first\n", cwd_buf);
            return LEUKO_EXIT_INVALID;
        }
    }

    char outdir_buf[PATH_MAX];
    if (outdir && outdir[0])
    {
        strncpy(outdir_buf, outdir, sizeof(outdir_buf) - 1);
        outdir_buf[sizeof(outdir_buf) - 1] = '\0';
    }
    else
    {
        int n = snprintf(outdir_buf, sizeof(outdir_buf), "%s/.leukocyte/configs", cwd_buf);
        if (n < 0 || (size_t)n >= sizeof(outdir_buf))
        {
            fprintf(stderr, "Outdir path construction failed\n");
            return LEUKO_EXIT_INVALID;
        }
    }

    char index_buf[PATH_MAX];
    if (index_path && index_path[0])
    {
        strncpy(index_buf, index_path, sizeof(index_buf) - 1);
        index_buf[sizeof(index_buf) - 1] = '\0';
    }
    else
    {
        int n = snprintf(index_buf, sizeof(index_buf), "%s/.leukocyte/index.json", cwd_buf);
        if (n < 0 || (size_t)n >= sizeof(index_buf))
        {
            fprintf(stderr, "Index path construction failed\n");
            return LEUKO_EXIT_INVALID;
        }
    }

    /* Build command and execute */
    char cmd[4096];
    int n = snprintf(cmd, sizeof(cmd), "ruby \"%s\" --dir \"%s\" --outdir \"%s\" --index \"%s\"", script, cwd_buf, outdir_buf, index_buf);
    if (n < 0 || (size_t)n >= sizeof(cmd))
    {
        fprintf(stderr, "Command construction failed\n");
        return LEUKO_EXIT_INVALID;
    }

    int rc = system(cmd);
    if (rc != 0)
    {
        fprintf(stderr, "Sync failed (rc=%d)\n", rc);
        return LEUKO_EXIT_INVALID;
    }

    return LEUKO_EXIT_OK;
}
