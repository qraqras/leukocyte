/* Integration test: ensure parser can run with arena enabled */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parse.h"
#include "allocator/prism_xallocator.h"

int main(void)
{
    /* Arena allocator is always enabled */

    /* Create a temporary Ruby-like file with many lines to exercise lexer */
    const char *path = "/tmp/leuko_test_large.rb";
    FILE *f = fopen(path, "w");
    if (!f)
        return 1;
    for (int i = 0; i < 20000; i++)
    {
        fprintf(f, "x = %d\n", i);
    }
    fclose(f);

    pm_node_t *node = NULL;
    pm_parser_t parser;
    uint8_t *source = NULL;
    /* token collection feature removed; parse without collecting tokens */
    int ok = parse_ruby_file(path, &node, &parser, &source);
    if (!ok)
    {
        fprintf(stderr, "parse failed\n");
        return 2;
    }

    /* cleanup: destroy node, free parser, and end arena */
    if (node)
        pm_node_destroy(&parser, node);
    pm_parser_free(&parser);
    x_allocator_end_parse();
    if (source)
        free(source);

    return 0;
}
