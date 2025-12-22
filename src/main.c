#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <inttypes.h>
#include <limits.h>

#include "cli/cli.h"
#include "parse.h"
#include "leukocyte.h"
#include "prism_xallocator.h"
#include "configs/config.h"
#include "configs/loader/loader.h"
#include "rules/rule_manager.h"
#include "prism/diagnostic.h"
#include "io/scan.h"
#include "worker/pipeline.h"
#include "worker/jobs.h"
#include "configs/discovery/discover.h"

/**
 * @brief Main entry point for Leuko application.
 * @param argc Argument count
 * @param argv Argument vector
 * @return Exit status code
 */
int main(int argc, char *argv[])
{
    // **** COMMAND LINE PARSING ****
    cli_options_t cli_opts = {0};
    int cp = parse_command_line(argc, argv, &cli_opts);
    if (cp != 0)
    {
        if (cp != 1)
        {
            fprintf(stderr, "Failed to parse command line arguments\n");
        }
        cli_options_free(&cli_opts);
        return EXIT_SUCCESS;
    }

    // **** CONFIGURATION LOADING ****
    config_t cfg = {0};
    initialize_config(&cfg);
    if (cli_opts.config_path)
    {
        char *cfg_err = NULL;
        if (!load_config_file_into(&cfg, cli_opts.config_path, &cfg_err))
        {
            fprintf(stderr, "Failed to load config file: %s\n", cfg_err ? cfg_err : cli_opts.config_path);
        }
        free(cfg_err);
    }

    init_rules();

    // **** RUBY FILE COLLECTION ****
    char **ruby_files = NULL;
    size_t ruby_files_count = 0;
    char *scan_err = NULL;

    if (!leuko_collect_ruby_files(cli_opts.paths, cli_opts.paths_count, &ruby_files, &ruby_files_count, &scan_err))
    {
        fprintf(stderr, "Error collecting Ruby files: %s\n", scan_err ? scan_err : "unknown");
        free(scan_err);
        cli_options_free(&cli_opts);
        free_config(&cfg);
        return EXIT_FAILURE;
    }
    if (ruby_files_count == 0)
    {
        fprintf(stderr, "Usage: %s [options] <ruby_file>\n", argv[0]);
        cli_options_free(&cli_opts);
        free_config(&cfg);
        free(ruby_files);
        return EXIT_FAILURE;
    }

    /* Parallel pipeline (deterministic ordering) */
    int any_failures = 0;

    /* Warm runtime config cache for discovered files to avoid per-file apply_config work. */
    {
        char *warm_err = NULL;
        size_t warm_workers = cli_opts.parallel ? leuko_num_workers() : 1;
        int wrc = leuko_config_warm_cache_for_files(ruby_files, ruby_files_count, warm_workers, &warm_err);
        if (wrc != 0)
        {
            fprintf(stderr, "Error: config warm failed: %s\n", warm_err ? warm_err : "unknown");
            free(warm_err);
            /* Treat warm failure as fatal to guarantee worker-only read-only access to cache */
            cli_options_free(&cli_opts);
            free_config(&cfg);
            for (size_t i = 0; i < ruby_files_count; ++i)
                free(ruby_files[i]);
            free(ruby_files);
            return EXIT_FAILURE;
        }
    }

    if (cli_opts.parallel)
    {
        /* Use worker pipeline */
        if (!leuko_run_pipeline(ruby_files, ruby_files_count, &cfg, leuko_num_workers(), &any_failures, (int)cli_opts.formatter))
        {
            fprintf(stderr, "Failed to run parallel pipeline\n");
            cli_options_free(&cli_opts);
            free_config(&cfg);
            for (size_t i = 0; i < ruby_files_count; ++i)
                free(ruby_files[i]);
            free(ruby_files);
            return EXIT_FAILURE;
        }
    }
    else
    {
        /* Sequential fallback (single-threaded behavior preserved) */
        for (size_t i = 0; i < ruby_files_count; ++i)
        {
            pm_node_t *root_node = NULL;
            pm_parser_t parser = {0};
            uint8_t *source = NULL;
            if (!leuko_parse_ruby_file(ruby_files[i], &root_node, &parser, &source))
            {
                fprintf(stderr, "Failed to parse Ruby file: %s\n", ruby_files[i]);
                any_failures = 1;
                continue;
            }

            // **** RULE APPLICATION ****
            /* Prefer warmed per-file config if available */
            const config_t *file_cfg = NULL;
            char *cerr = NULL;
            if (leuko_config_get_cached_config_for_file(ruby_files[i], &file_cfg, &cerr) != 0)
            {
                if (cerr)
                {
                    fprintf(stderr, "Warning: config discovery failed for %s: %s\n", ruby_files[i], cerr);
                    free(cerr);
                }
                file_cfg = NULL;
            }
            const config_t *used_cfg = file_cfg ? file_cfg : &cfg;

            const rules_by_type_t *rules = get_rules_by_type_for_file(used_cfg, ruby_files[i]);

            /* visit using used_cfg (cast away const for API) */
            visit_node_with_rules(root_node, &parser, NULL, (config_t *)used_cfg, rules);

            pm_node_destroy(&parser, root_node);
            pm_parser_free(&parser);
            /* End per-parse arena state after parser is freed so tokens remain available during rules */
            leuko_x_allocator_end();
            free(source);
        }
    }

    for (size_t i = 0; i < ruby_files_count; ++i)
    {
        free(ruby_files[i]);
    }
    free(ruby_files);

    /* debug: print number of freed diagnostics to verify pm_diagnostic_list_free behavior */
    extern size_t g_diag_freed;
    fprintf(stderr, "[debug] g_diag_freed=%zu\n", g_diag_freed);

    // TODO: Apply rules and perform analysis on root_node

    cli_options_free(&cli_opts);
    free_config(&cfg);

    return EXIT_SUCCESS;
}
