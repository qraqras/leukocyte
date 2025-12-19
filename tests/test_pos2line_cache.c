#include "io/processed_source.h"
#include "prism/parser.h"
#include "prism/util/pm_newline_list.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    const char *s = "foo\nbar\nbaz\n  qux\n";
    pm_parser_t parser = {0};
    parser.start = (const uint8_t *)s;
    parser.end = (const uint8_t *)(s + strlen(s));
    parser.start_line = 1;
    /* build a fake newline list */
    pm_newline_list_t nl = {0};
    size_t offsets[] = {0, 4, 8, 12};
    nl.size = 4;
    nl.offsets = offsets;
    parser.newline_list = nl;

    leuko_processed_source_t ps = {0};
    leuko_processed_source_init_from_parser(&ps, &parser);

    /* Query some positions multiple times and ensure cache returns same line */
    const uint8_t *p = parser.start + 6; /* in line 2 */
    leuko_processed_source_pos_info(&ps, p, &(leuko_processed_source_pos_info_t){0});
    /* Now ensure a subsequent get hits the cache (we can't inspect internal map directly) */
    leuko_processed_source_pos_info_t info;
    leuko_processed_source_pos_info(&ps, p, &info);
    assert(info.line_number == 2);

    leuko_processed_source_free(&ps);

    printf("offset2line cache smoke test passed\n");
    return 0;
}
