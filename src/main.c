#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cli/cli.h"
#include "parse.h"
#include "leukocyte.h"
#include "configs/generated_config.h"
#include "configs/loader.h"
#include "rules/rule_manager.h"
#include "io/scan.h"

/**
 * @brief Main entry point for Leuko application.
 * @param argc Argument count
 * @param argv Argument vector
 * @return Exit status code
 */
int main(int argc, char *argv[])
{
    fprintf(stderr, "Leuko: RuboCop Layout and Lint reimplementation in C\n");

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
        return EXIT_FAILURE;
    }
    if (ruby_files_count == 0)
    {
        fprintf(stderr, "Usage: %s [options] <ruby_file>\n", argv[0]);
        cli_options_free(&cli_opts);
        free(ruby_files);
        return EXIT_FAILURE;
    }

    // **** RUBY FILE PARSING ****
    int any_failures = 0;
    for (size_t i = 0; i < ruby_files_count; ++i)
    {
        pm_node_t *root_node = NULL;
        pm_parser_t parser = {0};
        uint8_t *source = NULL;
        if (!parse_ruby_file(ruby_files[i], &root_node, &parser, &source))
        {
            fprintf(stderr, "Failed to parse Ruby file: %s\n", ruby_files[i]);
            any_failures = 1;
            continue;
        }

        // **** RULE APPLICATION ****
        printf("Applying rules to file: %s\n", ruby_files[i]);
        visit_node(root_node, &parser, NULL, &cfg);

        pm_node_destroy(&parser, root_node);
        pm_parser_free(&parser);
        free(source);
    }

    for (size_t i = 0; i < ruby_files_count; ++i)
    {
        free(ruby_files[i]);
    }
    free(ruby_files);

    // TODO: Apply rules and perform analysis on root_node

    cli_options_free(&cli_opts);
    free_config(&cfg);

    return EXIT_SUCCESS;
}
