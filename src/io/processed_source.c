#include "io/processed_source.h"
#include <stddef.h>
#include <stdint.h>

/* Binary search helper: find the smallest index i such that offsets[i] > offset
 * returns i (0..size-1 or size if beyond end) */
static size_t bsearch_offsets(const size_t *offsets, size_t size, size_t offset)
{
    size_t left = 0;
    size_t right = (size == 0) ? 0 : size - 1;

    while (left <= right)
    {
        size_t mid = left + (right - left) / 2;
        if (offsets[mid] > offset)
        {
            if (mid == 0)
                return 0;
            right = mid - 1;
        }
        else
        {
            left = mid + 1;
        }
        if (right < left && left >= size)
            break;
    }
    return left;
}

void processed_source_init_from_parser(processed_source_t *ps, const pm_parser_t *parser)
{
    ps->newline_list = &parser->newline_list;
    ps->start = parser->start;
    ps->end = parser->end;
    ps->start_line = parser->start_line;
    /* number of offsets equals lines count (the offsets include a 0 entry) */
    ps->lines_count = ps->newline_list->size;

    /* Initialize caches */
    ps->last_line_idx = (size_t)-1;
    ps->pos2line_keys = NULL;
    ps->pos2line_vals = NULL;
    ps->pos2line_cap = 0;
    ps->pos2line_count = 0;

    /* line_start_offsets is optional; lazily initialized on first use */
    ps->line_start_offsets = NULL;

    /* Compute per-line first non-whitespace offsets (absolute offsets from start)
     * Allocate an array of length lines_count. If allocation fails, leave pointer NULL
     * and fall back to scanning on demand in other APIs. */
    ps->first_non_ws_offsets = NULL;
    if (ps->lines_count > 0)
    {
        ps->first_non_ws_offsets = (size_t *)xcalloc(ps->lines_count, sizeof(size_t));
        if (ps->first_non_ws_offsets)
        {
            const size_t *offs = ps->newline_list->offsets;
            for (size_t i = 0; i < ps->lines_count; ++i)
            {
                size_t line_begin = offs[i];
                size_t line_end = (i + 1 < ps->lines_count) ? offs[i + 1] : (size_t)(ps->end - ps->start);
                const uint8_t *p = ps->start + line_begin;
                size_t first = line_end; /* default: no non-ws found in line */
                while ((size_t)(p - ps->start) < line_end)
                {
                    if (*p != ' ' && *p != '\t')
                    {
                        first = (size_t)(p - ps->start);
                        break;
                    }
                    p++;
                }
                ps->first_non_ws_offsets[i] = first;
            }
        }
    }
}

int32_t processed_source_line_of_pos(const processed_source_t *ps, const uint8_t *pos)
{
    size_t offset = (size_t)(pos - ps->start);
    const size_t *offsets = ps->newline_list->offsets;
    size_t size = ps->newline_list->size;

    if (size == 0)
        return ps->start_line;

    size_t idx = bsearch_offsets(offsets, size, offset);
    /* idx is first offset > offset, so line index is idx - 1 (0-based) */
    if (idx == 0)
    {
        return ps->start_line; /* before first newline */
    }
    return (int32_t)((int32_t)(idx - 1) + ps->start_line);
}

size_t processed_source_col_of_pos(const processed_source_t *ps, const uint8_t *pos)
{
    size_t offset = (size_t)(pos - ps->start);
    const size_t *offsets = ps->newline_list->offsets;
    size_t size = ps->newline_list->size;

    if (size == 0)
        return offset;

    size_t idx = bsearch_offsets(offsets, size, offset);
    /* idx == 0 -> offset before first newline, column = offset */
    if (idx == 0)
        return offset;
    size_t line_idx0 = (idx == 0) ? 0 : (idx - 1);
    size_t line_begin = ps->line_start_offsets ? ps->line_start_offsets[line_idx0] : offsets[line_idx0];
    return (size_t)(offset - line_begin);
}

/* Simple open-addressing map for pos->line mapping. Capacity is power of two; key == SIZE_MAX means empty. */
static void processed_source_pos2line_ensure_capacity(processed_source_t *ps, size_t min_cap)
{
    if (ps->pos2line_cap >= min_cap)
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
    if (ps->pos2line_keys && ps->pos2line_cap > 0)
    {
        for (size_t i = 0; i < ps->pos2line_cap; ++i)
        {
            size_t k = ps->pos2line_keys[i];
            if (k == (size_t)-1)
                continue;
            size_t v = ps->pos2line_vals[i];
            size_t h = (k * 11400714819323198485ull) & (cap - 1);
            while (new_keys[h] != (size_t)-1)
                h = (h + 1) & (cap - 1);
            new_keys[h] = k;
            new_vals[h] = v;
        }
        xfree(ps->pos2line_keys);
        xfree(ps->pos2line_vals);
    }

    /* Note: count is rebuilt by scanning new_keys */
    size_t new_count = 0;
    for (size_t i = 0; i < cap; ++i)
        if (new_keys[i] != (size_t)-1)
            new_count++;
    ps->pos2line_count = new_count;

    ps->pos2line_keys = new_keys;
    ps->pos2line_vals = new_vals;
    ps->pos2line_cap = cap;
}

void processed_source_pos2line_put(processed_source_t *ps, size_t pos, size_t line_idx)
{
    /* Lazy allocate small table */
    if (ps->pos2line_cap == 0)
    {
        processed_source_pos2line_ensure_capacity(ps, 1024);
        if (ps->pos2line_cap == 0)
            return; /* allocation failed */
    }
    /* Grow if load factor exceeds ~0.7 */
    if (ps->pos2line_count * 10 >= ps->pos2line_cap * 7)
    {
        processed_source_pos2line_ensure_capacity(ps, ps->pos2line_cap * 2);
        if (ps->pos2line_cap == 0)
            return; /* allocation failed */
    }
    size_t cap = ps->pos2line_cap;
    size_t h = (pos * 11400714819323198485ull) & (cap - 1);
    while (ps->pos2line_keys[h] != (size_t)-1 && ps->pos2line_keys[h] != pos)
    {
        h = (h + 1) & (cap - 1);
    }
    if (ps->pos2line_keys[h] == (size_t)-1)
    {
        ps->pos2line_keys[h] = pos;
        ps->pos2line_vals[h] = line_idx;
        ps->pos2line_count += 1;
    }
    else
    {
        /* update existing */
        ps->pos2line_vals[h] = line_idx;
    }
}

int processed_source_pos2line_get(const processed_source_t *ps, size_t pos, size_t *out_line_idx)
{
    if (!ps || ps->pos2line_cap == 0 || !ps->pos2line_keys)
        return 0;
    size_t cap = ps->pos2line_cap;
    size_t h = (pos * 11400714819323198485ull) & (cap - 1);
    while (ps->pos2line_keys[h] != (size_t)-1)
    {
        if (ps->pos2line_keys[h] == pos)
        {
            *out_line_idx = ps->pos2line_vals[h];
            return 1;
        }
        h = (h + 1) & (cap - 1);
    }
    return 0;
}

static void processed_source_ensure_line_starts(processed_source_t *ps)
{
    if (!ps || ps->line_start_offsets)
        return;
    size_t n = ps->lines_count;
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

void processed_source_pos_info(processed_source_t *ps, const uint8_t *pos, processed_source_pos_info_t *out)
{
    size_t offset = (size_t)(pos - ps->start);
    const size_t *offsets = ps->newline_list->offsets;
    size_t size = ps->newline_list->size;

    if (size == 0)
    {
        out->line = ps->start_line;
        out->col = offset;
        out->begins = (offset == 0);
        return;
    }

    /* ensure optional line starts cache */
    processed_source_ensure_line_starts(ps);

    size_t line_idx0 = (size_t)-1;

    /* Fast path: check position->line cache */
    size_t cached_line = 0;
    if (processed_source_pos2line_get(ps, offset, &cached_line))
    {
        line_idx0 = cached_line;
    }

    /* Try using cached last_line_idx */
    if (line_idx0 == (size_t)-1 && ps->last_line_idx != (size_t)-1)
    {
        size_t li = ps->last_line_idx;
        size_t line_begin = offsets[li];
        size_t next_begin = (li + 1 < size) ? offsets[li + 1] : (size_t)(ps->end - ps->start);
        if (offset >= line_begin && offset < next_begin)
        {
            line_idx0 = li;
        }
    }

    if (line_idx0 == (size_t)-1)
    {
        size_t idx = bsearch_offsets(offsets, size, offset);
        line_idx0 = (idx == 0) ? 0 : (idx - 1);
        ps->last_line_idx = line_idx0;
        processed_source_pos2line_put(ps, offset, line_idx0);
    }

    out->line = (int32_t)(ps->start_line + (int32_t)line_idx0);

    size_t line_begin = ps->line_start_offsets ? ps->line_start_offsets[line_idx0] : offsets[line_idx0];
    out->col = (size_t)(offset - line_begin);

    if (ps->first_non_ws_offsets)
    {
        size_t first = ps->first_non_ws_offsets[line_idx0];
        out->begins = (offset == first);
    }
    else
    {
        /* fallback: scan from line_begin to pos checking whitespace */
        const uint8_t *p = ps->start + line_begin;
        bool ok = true;
        while (p < pos)
        {
            if (*p != ' ' && *p != '\t')
            {
                ok = false;
                break;
            }
            p++;
        }
        out->begins = ok;
    }
}

void processed_source_free(processed_source_t *ps)
{
    if (!ps)
        return;
    if (ps->first_non_ws_offsets)
    {
        xfree(ps->first_non_ws_offsets);
        ps->first_non_ws_offsets = NULL;
    }
    if (ps->line_start_offsets)
    {
        xfree(ps->line_start_offsets);
        ps->line_start_offsets = NULL;
    }
    if (ps->pos2line_keys)
    {
        xfree(ps->pos2line_keys);
        ps->pos2line_keys = NULL;
    }
    if (ps->pos2line_vals)
    {
        xfree(ps->pos2line_vals);
        ps->pos2line_vals = NULL;
    }
    ps->pos2line_cap = 0;
    ps->pos2line_count = 0;
}

bool processed_source_begins_its_line(const processed_source_t *ps, const uint8_t *pos)
{
    size_t offset = (size_t)(pos - ps->start);
    const size_t *offsets = ps->newline_list->offsets;
    size_t size = ps->newline_list->size;

    if (size == 0)
        return offset == 0;

    size_t idx = bsearch_offsets(offsets, size, offset);
    /* line begin offset (0-based) */
    size_t line_begin = (idx == 0) ? offsets[0] : offsets[idx - 1];

    /* Consider beginning its line if all bytes between line_begin and pos
     * are only spaces or tabs (i.e. pos is the first non-whitespace on the line) */
    const uint8_t *p = ps->start + line_begin;
    while (p < pos)
    {
        if (*p != ' ' && *p != '\t')
            return false;
        p++;
    }
    return true;
}
