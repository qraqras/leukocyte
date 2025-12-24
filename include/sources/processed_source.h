#ifndef LEUKO_PROCESSED_SOURCE_H
#define LEUKO_PROCESSED_SOURCE_H

#include <stddef.h>
#include <stdint.h>
#include "prism/util/pm_newline_list.h"
#include "prism/parser.h"

/**
 * @brief Processed source structure for efficient position lookups.
 */
typedef struct leuko_processed_source_s
{
    const pm_newline_list_t *newline_list;
    const uint8_t *source_start;
    const uint8_t *source_end;
    int32_t start_line_number;
    size_t line_count;
    size_t *line_first_non_ws_offsets;
    size_t *line_start_offsets;
    size_t last_line_index;
    size_t *offset2line_keys;
    size_t *offset2line_vals;
    size_t offset2line_cap;
    size_t offset2line_count;
} leuko_processed_source_t;

/**
 * @brief Position information within a processed source.
 * @note line_number is 1-based, column/indentation_column are 0-based.
 */
typedef struct leuko_processed_source_pos_info_s
{
    int32_t line_number;
    size_t column;
    size_t indentation_column;
} leuko_processed_source_pos_info_t;

void leuko_processed_source_init_from_parser(leuko_processed_source_t *ps, const pm_parser_t *parser);
int32_t leuko_processed_source_line_of_pos(const leuko_processed_source_t *ps, const uint8_t *pos);
size_t leuko_processed_source_col_of_pos(const leuko_processed_source_t *ps, const uint8_t *pos);
bool leuko_processed_source_begins_its_line(const leuko_processed_source_t *ps, const uint8_t *pos);
static inline size_t leuko_pos_to_offset(const leuko_processed_source_t *ps, const uint8_t *pos) { return (size_t)(pos - ps->source_start); }
static inline const uint8_t *leuko_offset_to_pos(const leuko_processed_source_t *ps, size_t offset) { return ps->source_start + offset; }

void leuko_processed_source_pos_info(leuko_processed_source_t *ps, const uint8_t *pos, leuko_processed_source_pos_info_t *out);
void leuko_processed_source_free(leuko_processed_source_t *ps);

#endif /* LEUKO_PROCESSED_SOURCE_H */
