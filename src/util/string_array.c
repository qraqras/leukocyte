/*
 * Utility: string array concatenation helper
 */
#include <stdlib.h>
#include <stdio.h>
#include "util/string_array.h"

/*
 * Concatenate `src` (array of `char*`) onto `*dest`. See header for ownership notes.
 */
bool leuko_string_array_concat(char ***dest, size_t *dest_count, char **src, size_t src_count)
{
    if (!src || src_count == 0)
    {
        return true;
    }
    if (!dest || !dest_count)
    {
        return false;
    }

    if (*dest == NULL)
    {
        *dest = src;
        *dest_count = src_count;
        return true;
    }

    char **tmp = realloc(*dest, (*dest_count + src_count) * sizeof(char *));
    if (!tmp)
    {
        return false;
    }
    *dest = tmp;
    for (size_t i = 0; i < src_count; i++)
    {
        (*dest)[*dest_count + i] = src[i];
    }
    *dest_count += src_count;
    free(src);
    return true;
}
