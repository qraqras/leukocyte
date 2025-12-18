#include <stdint.h>

#include "io/file.h"
#include "parse.h"
#include "prism.h"
#include "allocator/prism_xallocator.h"

/**
 * @brief Parse Ruby file at `filepath` into a Prism AST node.
 * @param filepath Ruby file path
 * @param out_node AST node output
 * @param out_parser Parser output
 * @param out_source Source buffer output
 * @return true on success, false on failure
 */
bool parse_ruby_file(const char *filepath, pm_node_t **out_node, pm_parser_t *out_parser, uint8_t **out_source)
{
    if (!filepath || !out_node || !out_parser)
    {
        return false;
    }

    /* Read file into buffer */
    uint8_t *source = NULL;
    size_t file_size = 0;
    char *read_err = NULL;
    if (!read_file_to_buffer(filepath, &source, &file_size, &read_err))
    {
        if (read_err)
        {
            fprintf(stderr, "failed to read '%s': %s\n", filepath, read_err);
            free(read_err);
        }
        return false;
    }

    /**
     * Initialize parser. The parser references the buffer but does not free it;
     * caller is responsible for freeing the buffer after pm_parser_free.
     */
    x_allocator_begin_parse();
    pm_parser_init(out_parser, source, file_size, NULL);
    *out_node = pm_parse(out_parser);

    /* Check for parse errors */
    if (out_parser->error_list.size > 0)
    {
        pm_node_destroy(out_parser, *out_node);
        pm_parser_free(out_parser);
        x_allocator_end_parse();
        free(source);
        return false;
    }

    if (out_source)
    {
        *out_source = source;
    }

    /* Note: do NOT free the parser or end the arena here on success; caller is
     * expected to call pm_parser_free() and then x_allocator_end_parse() so
     * that tokens remain available while rules run. */

    return true;
}
