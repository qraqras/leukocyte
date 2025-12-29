#include "cli/exit_code.h"
#include "cli/parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * @brief Entry point for CLI application.
 * @param argc Argument count
 * @param argv Argument vector
 * @return Exit code
 */
int main(int argc, char *argv[])
{
    leuko_cli_options_t cli_opts = {0};
    leuko_parse_result_t parse_result = leuko_cli_options_parse(argc, argv, &cli_opts);
    switch (parse_result)
    {
    case LEUKO_CLI_OPTIONS_PARSE_OK:
        break;
    case LEUKO_CLI_OPTIONS_PARSE_PRINTED:
        return LEUKO_EXIT_OK;
    case LEUKO_CLI_OPTIONS_PARSE_ERROR:
        return LEUKO_EXIT_INVALID;
    default:
        return LEUKO_EXIT_INVALID;
    }

    /* Handle sync subcommand */
    if (cli_opts.sync)
    {
        /* Determine config path by searching upward from current directory for .rubocop.yml */
        const char *script = "scripts/export_rubocop_config.rb";
        char cfgpath[1024];
        cfgpath[0] = '\0';
        {
            char cur[1024];
            if (!getcwd(cur, sizeof(cur)))
            {
                perror("getcwd");
                leuko_cli_options_free(&cli_opts);
                return LEUKO_EXIT_INVALID;
            }
            /* Inform the user of the current working directory for sync operation */
            printf("Current directory: %s\n", cur);
            for (;;)
            {
                char candidate[1024];
                snprintf(candidate, sizeof(candidate), "%s/.rubocop.yml", cur);
                if (access(candidate, R_OK) == 0)
                {
                    snprintf(cfgpath, sizeof(cfgpath), "%s", candidate);
                    break;
                }
                /* stop at root */
                if (strcmp(cur, "/") == 0)
                    break;
                char *p = strrchr(cur, '/');
                if (!p)
                    break;
                if (p == cur)
                {
                    /* reached root */
                    cur[1] = '\0';
                }
                else
                {
                    *p = '\0';
                }
            }
            if (cfgpath[0] == '\0')
            {
                fprintf(stderr, "No .rubocop.yml found in current directory or parents\n");
                leuko_cli_options_free(&cli_opts);
                return LEUKO_EXIT_INVALID;
            }
        }

        /* Determine output path in same directory as cfgpath */
        char outpath[1024];
        char *last_slash = strrchr(cfgpath, '/');
        if (last_slash)
        {
            size_t dirlen = last_slash - cfgpath;
            char dir[1024];
            if (dirlen >= sizeof(dir))
                dirlen = sizeof(dir) - 1;
            strncpy(dir, cfgpath, dirlen);
            dir[dirlen] = '\0';
            snprintf(outpath, sizeof(outpath), "%s/.leukocyte.resolved.json", dir);
        }
        else
        {
            snprintf(outpath, sizeof(outpath), ".leukocyte.resolved.json");
        }

        /* Ensure script exists */
        if (access(script, R_OK) != 0)
        {
            fprintf(stderr, "Sync script not found: %s\n", script);
            leuko_cli_options_free(&cli_opts);
            return LEUKO_EXIT_INVALID;
        }

        /* Build and run command */
        char cmd[4096];
        snprintf(cmd, sizeof(cmd), "ruby \"%s\" --config \"%s\" --out \"%s\"", script, cfgpath, outpath);
        int rc = system(cmd);
        if (rc != 0)
        {
            fprintf(stderr, "Sync failed (rc=%d)\n", rc);
            leuko_cli_options_free(&cli_opts);
            return LEUKO_EXIT_INVALID;
        }

        leuko_cli_options_free(&cli_opts);
        return LEUKO_EXIT_OK;
    }

    leuko_cli_options_free(&cli_opts);
    return LEUKO_EXIT_OK;
}
