#include <assert.h>
#include <string.h>
#include <stdbool.h>

#include "prism/parser.h"
#include "prism/util/pm_newline_list.h"
#include "io/processed_source.h"

int main(void)
{
    const char *src = "def foo\n  bar\n    baz\nend\nfoo bar\n";
    size_t len = strlen(src);

    pm_parser_t parser = {0};
    parser.start = (const uint8_t *)src;
    parser.end = parser.start + len;
    parser.start_line = 1;

    bool ok = pm_newline_list_init(&parser.newline_list, parser.start, 8);
    assert(ok);

    for (size_t i = 0; i < len; ++i)
    {
        if (src[i] == '\n')
        {
            ok = pm_newline_list_append(&parser.newline_list, parser.start + i);
            assert(ok);
        }
    }

    leuko_processed_source_t ps;
    leuko_processed_source_init_from_parser(&ps, &parser);

    /* start of file */
    const uint8_t *p0 = parser.start;
    assert(leuko_processed_source_line_of_pos(&ps, p0) == 1);
    assert(leuko_processed_source_col_of_pos(&ps, p0) == 0);
    assert(leuko_processed_source_begins_its_line(&ps, p0));

    /* position of 'bar' line: it has two leading spaces */
    const uint8_t *p_bar = (const uint8_t *)(src + 8); /* points to 'b' */
    assert(leuko_processed_source_line_of_pos(&ps, p_bar) == 2);
    assert(leuko_processed_source_col_of_pos(&ps, p_bar) == 2);
    assert(leuko_processed_source_begins_its_line(&ps, p_bar));

    /* position in middle of line '    baz' (4 spaces) check column */
    const uint8_t *p_baz = (const uint8_t *)(src + 13); /* points to 'b' of baz */
    assert(leuko_processed_source_line_of_pos(&ps, p_baz) == 3);
    assert(leuko_processed_source_col_of_pos(&ps, p_baz) == 4);
    assert(leuko_processed_source_begins_its_line(&ps, p_baz));

    /* position inside 'baz' but not at beginning (c of baz) */
    const uint8_t *p_baz_c = (const uint8_t *)(src + 14);
    assert(leuko_processed_source_line_of_pos(&ps, p_baz_c) == 3);
    assert(leuko_processed_source_col_of_pos(&ps, p_baz_c) == 5);
    assert(!leuko_processed_source_begins_its_line(&ps, p_baz_c));

    /* verify processed_source_pos_info matches expectations */
    leuko_processed_source_pos_info_t info;
    leuko_processed_source_pos_info(&ps, p0, &info);
    assert(info.line_number == 1 && info.column == 0 && info.indentation_column == 0);

    leuko_processed_source_pos_info(&ps, p_bar, &info);
    assert(info.line_number == 2 && info.column == 2 && info.indentation_column == 2);

    leuko_processed_source_pos_info(&ps, p_baz, &info);
    assert(info.line_number == 3 && info.column == 4 && info.indentation_column == 4);

    leuko_processed_source_pos_info(&ps, p_baz_c, &info);
    assert(info.line_number == 3 && info.column == 5 && info.indentation_column == 4);

    /* ensure a later token on the same line is not considered beginning */
    const char *p_foo_bar = strstr((const char *)parser.start, "foo bar");
    assert(p_foo_bar != NULL);
    const uint8_t *p_bar_late = (const uint8_t *)(p_foo_bar + 4); /* 'b' of 'bar' */
    assert(leuko_processed_source_line_of_pos(&ps, p_bar_late) == 6);
    assert(leuko_processed_source_col_of_pos(&ps, p_bar_late) == 4);
    assert(!leuko_processed_source_begins_its_line(&ps, p_bar_late));

    /* free processed_source allocations */
    leuko_processed_source_free(&ps);
    pm_newline_list_free(&parser.newline_list);

    return 0;
}
