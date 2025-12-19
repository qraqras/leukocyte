#ifndef LEUKO_PROCESSED_SOURCE_H
#define LEUKO_PROCESSED_SOURCE_H

#include <stddef.h>
#include <stdint.h>

#include "prism/util/pm_newline_list.h"
#include "prism/parser.h"

/**
 * @brief Processed source structure for efficient position lookups.
 */
typedef struct processed_source
{
    const pm_newline_list_t *newline_list;
    const uint8_t *start;
    const uint8_t *end;
    int32_t start_line;
    size_t lines_count;
    size_t *first_non_ws_offsets;
    size_t *line_start_offsets; /* optional cache: per-line start offsets (from start) */
    size_t last_line_idx;
    size_t *pos2line_keys;
    size_t *pos2line_vals;
    size_t pos2line_cap;
    size_t pos2line_count;
} processed_source_t;

/* Initialize processed_source from an existing parser. */
void processed_source_init_from_parser(processed_source_t *ps, const pm_parser_t *parser);

/* Return the 1-based line number for a given pointer into the source. */
int32_t processed_source_line_of_pos(const processed_source_t *ps, const uint8_t *pos);

/* Return the 0-based column for a given pointer into the source. */
size_t processed_source_col_of_pos(const processed_source_t *ps, const uint8_t *pos);

/* Return whether the given position is at the beginning of a line. */
bool processed_source_begins_its_line(const processed_source_t *ps, const uint8_t *pos);

/* Consolidated position info returned by a single lookup. */
typedef struct processed_source_pos_info_s
{
    int32_t line; /* 1-based */
    size_t col;   /* 0-based byte column */
    bool begins;  /* whether this pos is the first non-whitespace on the line */
} processed_source_pos_info_t;

/* Fill `out` with pos info in a single lookup (one bsearch + O(1) ops). */
void processed_source_pos_info(processed_source_t *ps, const uint8_t *pos, processed_source_pos_info_t *out);

/* Free any allocations owned by processed_source_t (safe to call on partially
 * initialized structures). */
void processed_source_free(processed_source_t *ps);

#endif /* LEUKO_PROCESSED_SOURCE_H */
