#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cli/cli.h"
#include "parse.h"
#include "leukocyte.h"
#include "prism_xallocator.h"
#include "configs/generated_config.h"
#include "configs/loader.h"
#include "rules/rule_manager.h"
#include "prism/diagnostic.h"
#include "io/scan.h"
#include <time.h>
#include <inttypes.h>

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
    bool res = collect_ruby_files(cli_opts.paths, cli_opts.paths_count, &ruby_files, &ruby_files_count, &scan_err);
    if (!res)
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

    // **** RUBY FILE PARSING ****
    int any_failures = 0;
    double total_parse_ms = 0.0;
    double total_build_ms = 0.0;
    double total_visit_ms = 0.0;
    uint64_t total_handler_ns = 0;
    for (size_t i = 0; i < ruby_files_count; ++i)
    {
        struct timespec t0, t1;
        pm_node_t *root_node = NULL;
        pm_parser_t parser = {0};
        uint8_t *source = NULL;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        if (!parse_ruby_file(ruby_files[i], &root_node, &parser, &source))
        {
            fprintf(stderr, "Failed to parse Ruby file: %s\n", ruby_files[i]);
            any_failures = 1;
            continue;
        }
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double parse_ms = ((t1.tv_sec - t0.tv_sec) * 1000.0) + ((t1.tv_nsec - t0.tv_nsec) / 1e6);
        total_parse_ms += parse_ms;

        // **** RULE APPLICATION ****
        clock_gettime(CLOCK_MONOTONIC, &t0);
        const rules_by_type_t *rules = get_rules_by_type_for_file(&cfg, ruby_files[i]);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double build_ms = ((t1.tv_sec - t0.tv_sec) * 1000.0) + ((t1.tv_nsec - t0.tv_nsec) / 1e6);
        total_build_ms += build_ms;

        /* Reset handler timing, run visit, then measure handler time separately */
        rule_manager_reset_timing();
        clock_gettime(CLOCK_MONOTONIC, &t0);
        visit_node_with_rules(root_node, &parser, NULL, &cfg, rules);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double visit_ms = ((t1.tv_sec - t0.tv_sec) * 1000.0) + ((t1.tv_nsec - t0.tv_nsec) / 1e6);
        total_visit_ms += visit_ms;

        uint64_t handler_ns = 0;
        size_t handler_calls = 0;
        rule_manager_get_timing(&handler_ns, &handler_calls);
        total_handler_ns += handler_ns;

        if (cli_opts.timings)
        {
            fprintf(stderr, "[timings] %s parse=%.3fms build_rules=%.3fms visit=%.3fms handler=%.3fms calls=%zu\n",
                    ruby_files[i], parse_ms, build_ms, visit_ms, handler_ns / 1e6, handler_calls);
            /* Machine-readable TIMING line for bench scripts */
            printf("TIMING file=%s parse_ms=%.3f visit_ms=%.3f handlers_ms=%.3f handler_calls=%zu\n",
                   ruby_files[i], parse_ms, visit_ms, handler_ns / 1e6, handler_calls);
        }

        pm_node_destroy(&parser, root_node);
        pm_parser_free(&parser);
        /* End per-parse arena state after parser is freed so tokens remain available during rules */
        x_allocator_end_parse();
        free(source);
    }

    for (size_t i = 0; i < ruby_files_count; ++i)
    {
        free(ruby_files[i]);
    }
    free(ruby_files);

    if (cli_opts.timings)
    {
        double total_parse = total_parse_ms;
        double total_build = total_build_ms;
        double total_visit = total_visit_ms;
        double total_handler_ms = total_handler_ns / 1e6;
        fprintf(stderr, "[timings] totals parse=%.3fms build_rules=%.3fms visit=%.3fms handler=%.3fms\n",
                total_parse, total_build, total_visit, total_handler_ms);
    }

    /* debug: print number of freed diagnostics to verify pm_diagnostic_list_free behavior */
    extern size_t g_diag_freed;
    fprintf(stderr, "[debug] g_diag_freed=%zu\n", g_diag_freed);

    // TODO: Apply rules and perform analysis on root_node

    cli_options_free(&cli_opts);
    free_config(&cfg);

    return EXIT_SUCCESS;
}
