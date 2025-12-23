#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "utils/string_array.h"

/**
 * @brief Concatenate `src` (array of `char*`) onto `*dest`.
 * @param dest Pointer to destination array of strings
 * @param dest_count Pointer to count of destination array
 * @param src Source array of strings
 * @param src_count Count of source array
 * @return true on success, false on failure
 */
bool leuko_str_arr_concat(char ***dest, size_t *dest_count, char **src, size_t src_count)
{
    if (!src || src_count == 0)
    {
        return true;
    }
    if (!dest || !dest_count)
    {
        return false;
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
    return true;
}

/**
 * @brief Push a single string `s` onto the array `*dest`.
 * @param dest Pointer to destination array of strings
 * @param dest_count Pointer to count of destination array
 * @param s String to push
 * @return true on success, false on failure
 */
bool leuko_str_arr_push(char ***dest, size_t *dest_count, const char *s)
{
    if (!dest || !dest_count || !s)
    {
        return false;
    }

    char *dup = strdup(s);
    if (!dup)
    {
        return false;
    }

    if (*dest == NULL)
    {
        *dest = calloc(1, sizeof(char *));
        if (!*dest)
        {
            free(dup);
            return false;
        }
        (*dest)[0] = dup;
        *dest_count = 1;
        return true;
    }

    char **tmp = realloc(*dest, (*dest_count + 1) * sizeof(char *));
    if (!tmp)
    {
        free(dup);
        return false;
    }
    *dest = tmp;
    (*dest)[(*dest_count)++] = dup;
    return true;
}

/**
 * @brief Split a string `str` by `delimiter` into an array of strings.
 * @param str Input string to split
 * @param delimiter Delimiter string
 * @param out Pointer to output array of strings
 * @param out_count Pointer to count of output array
 * @return true on success, false on failure
 */
bool leuko_str_arr_split(const char *str, const char *delimiter, char ***out, size_t *out_count)
{
    if (!out || !out_count || !delimiter || delimiter[0] == '\0')
    {
        return false;
    }

    if (!str || *str == '\0')
    {
        *out = NULL;
        *out_count = 0;
        return true;
    }

    size_t delim_len = strlen(delimiter);
    size_t count = 0;
    const char *p = str;

    /* Count occurrences to determine number of tokens */
    while ((p = strstr(p, delimiter)) != NULL)
    {
        ++count;
        p += delim_len;
    }

    size_t tokens = count + 1;
    char **arr = calloc(tokens, sizeof(char *));
    if (!arr)
    {
        *out = NULL;
        *out_count = 0;
        return false;
    }

    size_t idx = 0;
    const char *start = str;
    const char *match = NULL;
    /* Helper: trim in-place leading/trailing whitespace; returns trimmed length */
    auto trim_inplace = (size_t (*)(char *))NULL; /* placeholder for clarity */

    /* Process tokens and trim */
    while ((match = strstr(start, delimiter)) != NULL)
    {
        size_t len = match - start;
        char *tok = malloc(len + 1);
        if (!tok)
        {
            for (size_t j = 0; j < idx; ++j)
                free(arr[j]);
            free(arr);
            *out = NULL;
            *out_count = 0;
            return false;
        }
        memcpy(tok, start, len);
        tok[len] = '\0';

        /* Trim leading/trailing whitespace */
        char *s = tok;
        /* trim leading */
        while (*s && isspace((unsigned char)*s))
            ++s;
        char *e = tok + strlen(tok);
        while (e > s && isspace((unsigned char)*(e - 1)))
            --e;
        *e = '\0';
        if (s != tok)
        {
            memmove(tok, s, (e - s) + 1);
        }

        /* Skip empty tokens */
        if (tok[0] == '\0')
        {
            free(tok);
        }
        else
        {
            arr[idx++] = tok;
        }

        start = match + delim_len;
    }

    /* Last token */
    size_t len = strlen(start);
    char *tok = malloc(len + 1);
    if (!tok)
    {
        for (size_t j = 0; j < idx; ++j)
            free(arr[j]);
        free(arr);
        *out = NULL;
        *out_count = 0;
        return false;
    }
    memcpy(tok, start, len);
    tok[len] = '\0';

    /* Trim last token */
    char *s = tok;
    while (*s && isspace((unsigned char)*s))
        ++s;
    char *e = tok + strlen(tok);
    while (e > s && isspace((unsigned char)*(e - 1)))
        --e;
    *e = '\0';
    if (s != tok)
    {
        memmove(tok, s, (e - s) + 1);
    }

    if (tok[0] == '\0')
    {
        free(tok);
    }
    else
    {
        arr[idx++] = tok;
    }

    if (idx == 0)
    {
        free(arr);
        *out = NULL;
        *out_count = 0;
        return true;
    }

    *out = arr;
    *out_count = idx;
    return true;
}
