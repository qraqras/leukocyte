#include <stddef.h>
#include <stdint.h>

#include "io/processed_source.h"

/**
 * @brief Initialize processed_source from a parser.
 * @param ps Pointer to processed_source_t to initialize
 * @param parser Pointer to the pm_parser_t
 */
void leuko_processed_source_init_from_parser(leuko_processed_source_t *ps, const pm_parser_t *parser)
{
    ps->newline_list = &parser->newline_list;
    ps->source_start = parser->start;
    ps->source_end = parser->end;
    ps->start_line_number = parser->start_line;
    /* number of offsets equals line count (the offsets include a 0 entry) */
    ps->line_count = ps->newline_list->size;

    /* Initialize caches */
    ps->last_line_index = (size_t)-1;
    ps->offset2line_keys = NULL;
    ps->offset2line_vals = NULL;
    ps->offset2line_cap = 0;
    ps->offset2line_count = 0;

    /* line_start_offsets: eagerly initialize per-line start offsets (from source_start) */
    ps->line_start_offsets = NULL;
    if (ps->line_count > 0)
    {
        ps->line_start_offsets = (size_t *)xcalloc(ps->line_count, sizeof(size_t));
        if (ps->line_start_offsets)
        {
            const size_t *offs = ps->newline_list->offsets;
            for (size_t i = 0; i < ps->line_count; ++i)
            {
                ps->line_start_offsets[i] = offs[i];
            }
        }
    }

    /**
     * Compute per-line first non-whitespace offsets (absolute offsets from start)
     * Allocate an array of length lines_count. If allocation fails, leave pointer NULL
     * and fall back to scanning on demand in other APIs.
     */
    ps->line_first_non_ws_offsets = NULL;
    if (ps->line_count > 0)
    {
        ps->line_first_non_ws_offsets = (size_t *)xcalloc(ps->line_count, sizeof(size_t));
        if (ps->line_first_non_ws_offsets)
        {
            const size_t *offs = ps->newline_list->offsets;
            for (size_t i = 0; i < ps->line_count; ++i)
            {
                size_t line_begin = offs[i];
                size_t line_end = (i + 1 < ps->line_count) ? offs[i + 1] : (size_t)(ps->source_end - ps->source_start);
                const uint8_t *p = ps->source_start + line_begin;
                size_t first = line_end; /* default: no non-ws found in line */
                while ((size_t)(p - ps->source_start) < line_end)
                {
                    if (*p != ' ' && *p != '\t')
                    {
                        first = (size_t)(p - ps->source_start);
                        break;
                    }
                    p++;
                }
                ps->line_first_non_ws_offsets[i] = first;
            }
        }
    }
}

/**
 * @brief Free any allocations owned by leuko_processed_source_t (safe to call on partially initialized structures).
 * @param ps Pointer to leuko_processed_source_t to free
 * @param pos Position within the source
 * @return 1-based line number
 */
int32_t leuko_processed_source_line_of_pos(const leuko_processed_source_t *ps, const uint8_t *pos)
{
    size_t size = ps->newline_list->size;

    if (size == 0)
        return ps->start_line_number;

    return pm_newline_list_line(ps->newline_list, pos, ps->start_line_number);
}

/**
 * @brief Get the column number of a position within the processed source.
 * @param ps Pointer to leuko_processed_source_t
 * @param pos Position within the source
 * @return 0-based column number
 */
size_t leuko_processed_source_col_of_pos(const leuko_processed_source_t *ps, const uint8_t *pos)
{
    size_t size = ps->newline_list->size;

    if (size == 0)
        return (size_t)(pos - ps->source_start);

    pm_line_column_t lc = pm_newline_list_line_column(ps->newline_list, pos, ps->start_line_number);
    return (size_t)lc.column;
}

/**
 * @brief Check if a position begins its line (i.e., is at the first non-whitespace character).
 * @param ps Pointer to leuko_processed_source_t
 * @param pos Position within the source
 * @return true if position begins its line, false otherwise
 */
static void leuko_processed_source_offset2line_ensure_capacity(leuko_processed_source_t *ps, size_t min_cap)
{
    if (ps->offset2line_cap >= min_cap)
        return;
    size_t cap = 1;
    while (cap < min_cap)
        cap <<= 1;

    size_t *new_keys = (size_t *)xcalloc(cap, sizeof(size_t));
    size_t *new_vals = (size_t *)xcalloc(cap, sizeof(size_t));
    if (!new_keys || !new_vals)
    {
        if (new_keys)
            xfree(new_keys);
        if (new_vals)
            xfree(new_vals);
        return;
    }

    /* initialize keys to empty */
    for (size_t i = 0; i < cap; ++i)
        new_keys[i] = (size_t)-1;

    /* If we had existing entries, rehash them into new arrays */
    if (ps->offset2line_keys && ps->offset2line_cap > 0)
    {
        for (size_t i = 0; i < ps->offset2line_cap; ++i)
        {
            size_t k = ps->offset2line_keys[i];
            if (k == (size_t)-1)
                continue;
            size_t v = ps->offset2line_vals[i];
            size_t h = (k * 11400714819323198485ull) & (cap - 1);
            while (new_keys[h] != (size_t)-1)
                h = (h + 1) & (cap - 1);
            new_keys[h] = k;
            new_vals[h] = v;
        }
        xfree(ps->offset2line_keys);
        xfree(ps->offset2line_vals);
    }

    /* Note: count is rebuilt by scanning new_keys */
    size_t new_count = 0;
    for (size_t i = 0; i < cap; ++i)
        if (new_keys[i] != (size_t)-1)
            new_count++;
    ps->offset2line_count = new_count;

    ps->offset2line_keys = new_keys;
    ps->offset2line_vals = new_vals;
    ps->offset2line_cap = cap;
}

/**
 * @brief Insert or update the offset-to-line cache.
 * @param ps Pointer to leuko_processed_source_t
 * @param offset Offset within the source
 * @param line_idx 0-based line index
 */
void leuko_processed_source_offset2line_put(leuko_processed_source_t *ps, size_t offset, size_t line_idx)
{
    /* Lazy allocate small table */
    if (ps->offset2line_cap == 0)
    {
        leuko_processed_source_offset2line_ensure_capacity(ps, 1024);
        if (ps->offset2line_cap == 0)
            return; /* allocation failed */
    }
    /* Grow if load factor exceeds ~0.7 */
    if (ps->offset2line_count * 10 >= ps->offset2line_cap * 7)
    {
        leuko_processed_source_offset2line_ensure_capacity(ps, ps->offset2line_cap * 2);
        if (ps->offset2line_cap == 0)
            return; /* allocation failed */
    }
    size_t cap = ps->offset2line_cap;
    size_t h = (offset * 11400714819323198485ull) & (cap - 1);
    while (ps->offset2line_keys[h] != (size_t)-1 && ps->offset2line_keys[h] != offset)
    {
        h = (h + 1) & (cap - 1);
    }
    if (ps->offset2line_keys[h] == (size_t)-1)
    {
        ps->offset2line_keys[h] = offset;
        ps->offset2line_vals[h] = line_idx;
        ps->offset2line_count += 1;
    }
    else
    {
        /* update existing */
        ps->offset2line_vals[h] = line_idx;
    }
}

/**
 * @brief Retrieve the line index for a given offset from the cache.
 * @param ps Pointer to leuko_processed_source_t
 * @param offset Offset within the source
 * @param out_line_idx Pointer to store the 0-based line index
 * @return 1 if found, 0 otherwise
 */
int leuko_processed_source_offset2line_get(const leuko_processed_source_t *ps, size_t offset, size_t *out_line_idx)
{
    if (!ps || ps->offset2line_cap == 0 || !ps->offset2line_keys)
        return 0;
    size_t cap = ps->offset2line_cap;
    size_t h = (offset * 11400714819323198485ull) & (cap - 1);
    while (ps->offset2line_keys[h] != (size_t)-1)
    {
        if (ps->offset2line_keys[h] == offset)
        {
            *out_line_idx = ps->offset2line_vals[h];
            return 1;
        }
        h = (h + 1) & (cap - 1);
    }
    return 0;
}

/**
 * @brief Ensure that line_start_offsets is initialized.
 * @param ps Pointer to leuko_processed_source_t
 */
static void leuko_processed_source_ensure_line_starts(leuko_processed_source_t *ps)
{
    if (!ps || ps->line_start_offsets)
        return;
    size_t n = ps->line_count;
    if (n == 0)
        return;
    const size_t *offs = ps->newline_list->offsets;
    size_t *arr = (size_t *)xcalloc(n, sizeof(size_t));
    if (!arr)
        return; /* allocation failed, leave NULL */
    for (size_t i = 0; i < n; ++i)
        arr[i] = offs[i];
    ps->line_start_offsets = arr;
}

/**
 * @brief Get detailed position information within the processed source.
 * @param ps Pointer to leuko_processed_source_t
 * @param pos Position within the source
 * @param out Pointer to leuko_processed_source_pos_info_t to fill
 */
void leuko_processed_source_pos_info(leuko_processed_source_t *ps, const uint8_t *pos, leuko_processed_source_pos_info_t *out)
{
    size_t offset = (size_t)(pos - ps->source_start);
    const size_t *offsets = ps->newline_list->offsets;
    size_t size = ps->newline_list->size;

    if (size == 0)
    {
        out->line_number = ps->start_line_number;
        out->column = offset;
        /* compute indentation column for single-line source */
        size_t fc = 0;
        const uint8_t *p = ps->source_start;
        while (p < ps->source_end)
        {
            if (*p != ' ' && *p != '\t')
                break;
            p++;
            fc++;
        }
        out->indentation_column = fc;
        return;
    }

    /* ensure optional line starts cache */
    leuko_processed_source_ensure_line_starts(ps);

    size_t line_idx0 = (size_t)-1;

    /* Fast path: check position->line cache */
    size_t cached_line = 0;
    if (leuko_processed_source_offset2line_get(ps, offset, &cached_line))
    {
        line_idx0 = cached_line;
    }

    /* Try using cached last_line_index */
    if (line_idx0 == (size_t)-1 && ps->last_line_index != (size_t)-1)
    {
        size_t li = ps->last_line_index;
        size_t line_begin = offsets[li];
        size_t next_begin = (li + 1 < size) ? offsets[li + 1] : (size_t)(ps->source_end - ps->source_start);
        if (offset >= line_begin && offset < next_begin)
        {
            line_idx0 = li;
        }
    }

    if (line_idx0 == (size_t)-1)
    {
        pm_line_column_t lc = pm_newline_list_line_column(ps->newline_list, ps->source_start + offset, ps->start_line_number);
        line_idx0 = (size_t)(lc.line - ps->start_line_number);
        ps->last_line_index = line_idx0;
        leuko_processed_source_offset2line_put(ps, offset, line_idx0);
    }

    out->line_number = (int32_t)(ps->start_line_number + (int32_t)line_idx0);

    size_t line_begin = ps->line_start_offsets ? ps->line_start_offsets[line_idx0] : offsets[line_idx0];
    out->column = (size_t)(offset - line_begin);

    /* Determine indentation column (0-based) */
    if (ps->line_first_non_ws_offsets)
    {
        size_t first_off = ps->line_first_non_ws_offsets[line_idx0];
        out->indentation_column = first_off - line_begin;
    }
    else
    {
        size_t line_end = (line_idx0 + 1 < size) ? offsets[line_idx0 + 1] : (size_t)(ps->source_end - ps->source_start);
        const uint8_t *p = ps->source_start + line_begin;
        size_t fc = 0;
        while ((size_t)(p - ps->source_start) < line_end)
        {
            if (*p != ' ' && *p != '\t')
                break;
            p++;
            fc++;
        }
        out->indentation_column = fc;
    }
}

/**
 * @brief Free any allocations owned by leuko_processed_source_t (safe to call on partially initialized structures).
 * @param ps Pointer to leuko_processed_source_t to free
 */
void leuko_processed_source_free(leuko_processed_source_t *ps)
{
    if (!ps)
        return;
    if (ps->line_first_non_ws_offsets)
    {
        xfree(ps->line_first_non_ws_offsets);
        ps->line_first_non_ws_offsets = NULL;
    }
    if (ps->line_start_offsets)
    {
        xfree(ps->line_start_offsets);
        ps->line_start_offsets = NULL;
    }
    if (ps->offset2line_keys)
    {
        xfree(ps->offset2line_keys);
        ps->offset2line_keys = NULL;
    }
    if (ps->offset2line_vals)
    {
        xfree(ps->offset2line_vals);
        ps->offset2line_vals = NULL;
    }
    ps->offset2line_cap = 0;
    ps->offset2line_count = 0;
}

bool leuko_processed_source_begins_its_line(const leuko_processed_source_t *ps, const uint8_t *pos)
{
    size_t offset = (size_t)(pos - ps->source_start);
    const size_t *offsets = ps->newline_list->offsets;
    size_t size = ps->newline_list->size;

    if (size == 0)
        return offset == 0;

    int32_t line = pm_newline_list_line(ps->newline_list, pos, ps->start_line_number);
    size_t line_idx0 = (size_t)(line - ps->start_line_number);
    /* line begin offset (0-based) */
    size_t line_begin = offsets[line_idx0];

    /* Consider beginning its line if all bytes between line_begin and pos
     * are only spaces or tabs (i.e. pos is the first non-whitespace on the line) */
    const uint8_t *p = ps->source_start + line_begin;
    while (p < pos)
    {
        if (*p != ' ' && *p != '\t')
            return false;
        p++;
    }
    return true;
}
