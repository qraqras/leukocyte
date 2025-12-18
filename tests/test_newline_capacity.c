/* Test that pm_newline_list initial capacity is at least the newline count + 2 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "prism.h"

int main(void)
{
    /* build a buffer with a known number of newlines */
    size_t lines = 10000;
    size_t buflen = lines * 3 + 10; /* e.g., "x\n" repeated */
    char *buf = malloc(buflen + 1);
    if (!buf) return 2;
    char *cur = buf;
    for (size_t i = 0; i < lines; i++) {
        *cur++ = 'x';
        *cur++ = '\n';
        *cur++ = 'a';
    }
    *cur = '\0';

    pm_parser_t parser;
    pm_parser_init(&parser, (const uint8_t *)buf, strlen(buf), NULL);

    /* Run the parser helper to adjust capacity and assert it grew as expected */
    leuko_adjust_newline_capacity(&parser, (const uint8_t *)buf, strlen(buf));
    size_t capacity = parser.newline_list.capacity;
    /* expect at least lines + 2 */
    assert(capacity >= lines + 2);

    pm_parser_free(&parser);
    free(buf);
    return 0;
}
