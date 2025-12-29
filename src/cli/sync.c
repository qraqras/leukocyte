#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <uv.h>
#include "cli/sync.h"

#ifndef PATH_MAX
#define LEUKO_PATH_MAX 4096
#else
#define LEUKO_PATH_MAX PATH_MAX
#endif

int leuko_sync_run(const char *script_path, const char *out_name)
{
    const char *script = script_path ? script_path : "scripts/export_rubocop_config.rb";
    const char *outname = out_name ? out_name : ".leukocyte.resolved.json";

    /* Always start from the current working directory */
    char cur[LEUKO_PATH_MAX];
    if (!getcwd(cur, sizeof(cur)))
    {
        perror("getcwd");
        return LEUKO_EXIT_INVALID;
    }

    char cfgpath[LEUKO_PATH_MAX];
    cfgpath[0] = '\0';

    uv_loop_t loop;
    if (uv_loop_init(&loop) != 0)
    {
        fprintf(stderr, "uv_loop_init failed\n");
        return LEUKO_EXIT_INVALID;
    }

    /* Helper: check existence using libuv stat (implemented inline below) */

    /* Find project root by searching for Gemfile or gems.rb */
    char project_root[LEUKO_PATH_MAX];
    project_root[0] = '\0';
    {
        /* reuse cur as traversal buffer */
        char tcur[LEUKO_PATH_MAX];
        strncpy(tcur, cur, sizeof(tcur) - 1);
        tcur[sizeof(tcur) - 1] = '\0';
        for (;;)
        {
            char cand[LEUKO_PATH_MAX];
            int nn = snprintf(cand, sizeof(cand), "%s/Gemfile", tcur);
            if (nn >= 0 && (size_t)nn < sizeof(cand))
            {
                uv_fs_t rreq;
                int rr = uv_fs_stat(&loop, &rreq, cand, NULL);
                if ((rr == 0 && rreq.result >= 0) || rreq.result >= 0)
                {
                    /* project root is directory containing Gemfile */
                    size_t len = strrchr(cand, '/') - cand;
                    if (len >= sizeof(project_root))
                        len = sizeof(project_root) - 1;
                    strncpy(project_root, cand, len);
                    project_root[len] = '\0';
                    uv_fs_req_cleanup(&rreq);
                    break;
                }
                uv_fs_req_cleanup(&rreq);
            }
            nn = snprintf(cand, sizeof(cand), "%s/gems.rb", tcur);
            if (nn >= 0 && (size_t)nn < sizeof(cand))
            {
                uv_fs_t rreq2;
                int rr2 = uv_fs_stat(&loop, &rreq2, cand, NULL);
                if ((rr2 == 0 && rreq2.result >= 0) || rreq2.result >= 0)
                {
                    size_t len = strrchr(cand, '/') - cand;
                    if (len >= sizeof(project_root))
                        len = sizeof(project_root) - 1;
                    strncpy(project_root, cand, len);
                    project_root[len] = '\0';
                    uv_fs_req_cleanup(&rreq2);
                    break;
                }
                uv_fs_req_cleanup(&rreq2);
            }
            if (strcmp(tcur, "/") == 0)
                break;
            char *p2 = strrchr(tcur, '/');
            if (!p2)
                break;
            if (p2 == tcur)
                tcur[1] = '\0';
            else
                *p2 = '\0';
        }
    }

    /* 1) find .rubocop.yml upwards stopping at project_root (if set) */
    int found = 0;
    {
        char tcur[LEUKO_PATH_MAX];
        strncpy(tcur, cur, sizeof(tcur) - 1);
        tcur[sizeof(tcur) - 1] = '\0';
        for (;;)
        {
            char cand[LEUKO_PATH_MAX];
            int nn = snprintf(cand, sizeof(cand), "%s/.rubocop.yml", tcur);
            if (nn < 0 || (size_t)nn >= sizeof(cand))
                break;
            uv_fs_t rreq;
            int rr = uv_fs_stat(&loop, &rreq, cand, NULL);
            if ((rr == 0 && rreq.result >= 0) || rreq.result >= 0)
            {
                strncpy(cfgpath, cand, sizeof(cfgpath) - 1);
                cfgpath[sizeof(cfgpath) - 1] = '\0';
                uv_fs_req_cleanup(&rreq);
                found = 1;
                break;
            }
            uv_fs_req_cleanup(&rreq);
            if (project_root[0] && strcmp(tcur, project_root) == 0)
                break;
            if (strcmp(tcur, "/") == 0)
                break;
            char *p2 = strrchr(tcur, '/');
            if (!p2)
                break;
            if (p2 == tcur)
                tcur[1] = '\0';
            else
                *p2 = '\0';
        }
    }

    /* 2) project root .config/.rubocop.yml or .config/rubocop/config.yml */
    if (!found && project_root[0])
    {
        char cand[LEUKO_PATH_MAX];
        int nn = snprintf(cand, sizeof(cand), "%s/.config/.rubocop.yml", project_root);
        if (nn >= 0 && (size_t)nn < sizeof(cand))
        {
            uv_fs_t rreq;
            int rr = uv_fs_stat(&loop, &rreq, cand, NULL);
            if ((rr == 0 && rreq.result >= 0) || rreq.result >= 0)
            {
                strncpy(cfgpath, cand, sizeof(cfgpath) - 1);
                cfgpath[sizeof(cfgpath) - 1] = '\0';
                uv_fs_req_cleanup(&rreq);
                found = 1;
            }
            else
                uv_fs_req_cleanup(&rreq);
        }
        if (!found)
        {
            int nn2 = snprintf(cand, sizeof(cand), "%s/.config/rubocop/config.yml", project_root);
            if (nn2 >= 0 && (size_t)nn2 < sizeof(cand))
            {
                uv_fs_t rreq2;
                int rr2 = uv_fs_stat(&loop, &rreq2, cand, NULL);
                if ((rr2 == 0 && rreq2.result >= 0) || rreq2.result >= 0)
                {
                    strncpy(cfgpath, cand, sizeof(cfgpath) - 1);
                    cfgpath[sizeof(cfgpath) - 1] = '\0';
                    uv_fs_req_cleanup(&rreq2);
                    found = 1;
                }
                else
                    uv_fs_req_cleanup(&rreq2);
            }
        }
    }

    /* 3) user's home ~/.rubocop.yml */
    if (!found)
    {
        const char *home = getenv("HOME");
        if (home)
        {
            char cand[LEUKO_PATH_MAX];
            int nn = snprintf(cand, sizeof(cand), "%s/.rubocop.yml", home);
            if (nn >= 0 && (size_t)nn < sizeof(cand))
            {
                uv_fs_t rreq;
                int rr = uv_fs_stat(&loop, &rreq, cand, NULL);
                if ((rr == 0 && rreq.result >= 0) || rreq.result >= 0)
                {
                    strncpy(cfgpath, cand, sizeof(cfgpath) - 1);
                    cfgpath[sizeof(cfgpath) - 1] = '\0';
                    uv_fs_req_cleanup(&rreq);
                    found = 1;
                }
                else
                    uv_fs_req_cleanup(&rreq);
            }
        }
    }

    /* 4) XDG config: $XDG_CONFIG_HOME/rubocop/config.yml or ~/.config/rubocop/config.yml */
    if (!found)
    {
        const char *xdg = getenv("XDG_CONFIG_HOME");
        char cand[LEUKO_PATH_MAX];
        if (xdg && xdg[0])
        {
            int nn = snprintf(cand, sizeof(cand), "%s/rubocop/config.yml", xdg);
            if (nn >= 0 && (size_t)nn < sizeof(cand))
            {
                uv_fs_t rreq;
                int rr = uv_fs_stat(&loop, &rreq, cand, NULL);
                if ((rr == 0 && rreq.result >= 0) || rreq.result >= 0)
                {
                    strncpy(cfgpath, cand, sizeof(cfgpath) - 1);
                    cfgpath[sizeof(cfgpath) - 1] = '\0';
                    uv_fs_req_cleanup(&rreq);
                    found = 1;
                }
                else
                    uv_fs_req_cleanup(&rreq);
            }
        }
        if (!found)
        {
            const char *home = getenv("HOME");
            if (home)
            {
                int nn = snprintf(cand, sizeof(cand), "%s/.config/rubocop/config.yml", home);
                if (nn >= 0 && (size_t)nn < sizeof(cand))
                {
                    uv_fs_t rreq2;
                    int rr2 = uv_fs_stat(&loop, &rreq2, cand, NULL);
                    if ((rr2 == 0 && rreq2.result >= 0) || rreq2.result >= 0)
                    {
                        strncpy(cfgpath, cand, sizeof(cfgpath) - 1);
                        cfgpath[sizeof(cfgpath) - 1] = '\0';
                        uv_fs_req_cleanup(&rreq2);
                        found = 1;
                    }
                    else
                        uv_fs_req_cleanup(&rreq2);
                }
            }
        }
    }

    if (!found)
    {
        fprintf(stderr, "No .rubocop.yml found (searched project, user XDG and home locations)\n");
        uv_loop_close(&loop);
        return LEUKO_EXIT_INVALID;
    }

    /* Determine output path in same directory as cfgpath */
    char outpath[LEUKO_PATH_MAX];
    char *last_slash = strrchr(cfgpath, '/');
    if (last_slash)
    {
        size_t dirlen = last_slash - cfgpath;
        char dir[LEUKO_PATH_MAX];
        if (dirlen >= sizeof(dir))
            dirlen = sizeof(dir) - 1;
        strncpy(dir, cfgpath, dirlen);
        dir[dirlen] = '\0';
        int n = snprintf(outpath, sizeof(outpath), "%s/%s", dir, outname);
        if (n < 0 || (size_t)n >= sizeof(outpath))
        {
            fprintf(stderr, "Path too long for outpath\n");
            return LEUKO_EXIT_INVALID;
        }
    }
    else
    {
        int n = snprintf(outpath, sizeof(outpath), "%s", outname);
        if (n < 0 || (size_t)n >= sizeof(outpath))
        {
            fprintf(stderr, "Path too long for outpath\n");
            return LEUKO_EXIT_INVALID;
        }
    }

    /* Ensure script exists */
    {
        uv_fs_t sreq;
        int r = uv_fs_stat(&loop, &sreq, script, NULL);
        if (!((r == 0 && sreq.result >= 0) || sreq.result >= 0))
        {
            uv_fs_req_cleanup(&sreq);
            fprintf(stderr, "Sync script not found: %s\n", script);
            uv_loop_close(&loop);
            return LEUKO_EXIT_INVALID;
        }
        uv_fs_req_cleanup(&sreq);
    }

    /* Build and run command */
    char cmd[4096];
    int n = snprintf(cmd, sizeof(cmd), "ruby \"%s\" --config \"%s\" --out \"%s\"", script, cfgpath, outpath);
    if (n < 0 || (size_t)n >= sizeof(cmd))
    {
        fprintf(stderr, "Command construction failed or too long\n");
        uv_loop_close(&loop);
        return LEUKO_EXIT_INVALID;
    }

    int rc = system(cmd);
    uv_loop_close(&loop);
    if (rc != 0)
    {
        fprintf(stderr, "Sync failed (rc=%d)\n", rc);
        return LEUKO_EXIT_INVALID;
    }

    return LEUKO_EXIT_OK;
}
