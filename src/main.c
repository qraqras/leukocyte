#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "leukocyte.h"

#include "prism.h"

#include "rules/rule_manager.h"
#include "cli.h"
#include "configs/config.h"

bool parse_ruby_file(const char *filepath, pm_node_t **out_node, pm_parser_t *out_parser)
{
    FILE *file = fopen(filepath, "rb");
    if (!file)
    {
        return false;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size < 0)
    {
        fclose(file);
        return false;
    }

    // Read file content
    uint8_t *source = (uint8_t *)malloc(file_size);
    if (!source)
    {
        fclose(file);
        return false;
    }

    size_t read_size = fread(source, 1, file_size, file);
    fprintf(stderr, "DEBUG: file_size=%ld read_size=%zu\n", file_size, read_size);
    fclose(file);

    if (read_size != (size_t)file_size)
    {
        free(source);
        return false;
    }

    // Initialize parser
    pm_parser_init(out_parser, source, file_size, NULL);

    // Parse
    *out_node = pm_parse(out_parser);

    fprintf(stderr, "DEBUG: file_size=%ld read_size=%zu error_list_size=%zu\n", file_size, read_size, out_parser->error_list.size);

    // Check for errors
    if (out_parser->error_list.size > 0)
    {
        fprintf(stderr, "Parse errors found:\n");
        pm_diagnostic_t *error = (pm_diagnostic_t *)out_parser->error_list.head;
        while (error)
        {
            fprintf(stderr, "Error: %s\n", error->message);
            error = (pm_diagnostic_t *)error->node.next;
        }
        pm_node_destroy(out_parser, *out_node);
        pm_parser_free(out_parser);
        free(source);
        return false;
    }

    // Pretty print the AST (for demonstration)
    pm_buffer_t buffer = {0};
    pm_prettyprint(&buffer, out_parser, *out_node);
    // Null terminate the buffer
    pm_buffer_append_string(&buffer, "", 1);
    fprintf(stderr, "Parsed AST:\n%s", buffer.value);
    pm_buffer_free(&buffer);

    // Clean up source is not needed, parser holds it
    // free(source);

    return true;
}

int main(int argc, char *argv[])
{
    fprintf(stderr, "Leuko: RuboCop Layout and Lint reimplementation in C\n");

    // CLI argument parsing
    cli_options_t opts;
    int cp = cli_parse(argc, argv, &opts);
    if (cp == 1)
    {
        // help or version printed
        cli_options_free(&opts);
        return EXIT_SUCCESS;
    }
    if (cp != 0)
    {
        fprintf(stderr, "Failed to parse command line arguments\n");
        return EXIT_FAILURE;
    }

    const char *filepath = NULL;
    if (opts.paths_count > 0)
        filepath = opts.paths[0];
    if (!filepath)
    {
        fprintf(stderr, "Usage: %s [options] <ruby_file>\n", argv[0]);
        cli_options_free(&opts);
        return EXIT_FAILURE;
    }

    config_t *cfg = NULL;
    if (opts.config_path)
    {
        cfg = config_load_from_file(opts.config_path);
    }
    pm_parser_t *parser = (pm_parser_t *)malloc(sizeof(pm_parser_t));
    if (!parser)
    {
        fprintf(stderr, "Failed to allocate parser\n");
        return EXIT_FAILURE;
    }
    pm_node_t *node;
    if (!parse_ruby_file(filepath, &node, parser))
    {
        // parse_ruby_file already frees parser's internal state via pm_parser_free()
        free(parser);
        if (cfg)
            config_free(cfg);
        cli_options_free(&opts);
        fprintf(stderr, "Failed to parse Ruby file: %s\n", filepath);
        return EXIT_FAILURE;
    }

    fprintf(stderr, "Successfully parsed Ruby file: %s\n", filepath);

    // Initialize rules
    init_rules();

    // Visit AST for rule checking
    pm_list_t diagnostics = {0};
    visit_node(node, parser, &diagnostics, cfg);

    // Output diagnostics
    if (diagnostics.size > 0)
    {
        fprintf(stderr, "Diagnostics found:\n");
        pm_diagnostic_t *diag = (pm_diagnostic_t *)diagnostics.head;
        while (diag)
        {
            fprintf(stderr, "Diagnostic: %s at position %ld\n", diag->message, (long)(diag->location.start - parser->start));
            diag = (pm_diagnostic_t *)diag->node.next;
        }
    }
    else
    {
        fprintf(stderr, "No diagnostics found.\n");
    }
    fflush(stderr);

    // Clean up
    uint8_t *source = (uint8_t *)parser->start;
    pm_node_destroy(parser, node);
    free(source);
    pm_parser_free(parser);
    free(parser);
    // Free diagnostics messages
    pm_diagnostic_t *diag = (pm_diagnostic_t *)diagnostics.head;
    while (diag)
    {
        if (diag->owned && diag->message)
        {
            free((void *)diag->message);
        }
        diag = (pm_diagnostic_t *)diag->node.next;
    }

    pm_list_free(&diagnostics);
    if (cfg)
        config_free(cfg);
    cli_options_free(&opts);

    return EXIT_SUCCESS;
}
