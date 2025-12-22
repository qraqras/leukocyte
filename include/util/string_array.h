/*
 * Utility functions for manipulating arrays of C strings (char **)
 */
#ifndef LEUKO_UTIL_STRING_ARRAY_H
#define LEUKO_UTIL_STRING_ARRAY_H

#include <stdbool.h>
#include <stddef.h>

/*
 * Concatenate `src` (an array of `char *` of length `src_count`) onto `*dest`.
 * Behavior:
 * - If `src` is NULL or `src_count` is 0, returns true (no-op).
 * - If `dest` or `dest_count` is NULL, returns false.
 * - If `*dest` is NULL, the `src` pointer is adopted directly and `*dest_count` is set to `src_count`.
 * - On success, the `src` pointer is freed (but the strings it points to are not freed) and ownership
 *   of the element pointers is transferred to `*dest`.
 * - On allocation failure, returns false and leaves `*dest` and `src` unchanged; the caller remains
 *   responsible for freeing `src`.
 */
bool leuko_string_array_concat(char ***dest, size_t *dest_count, char **src, size_t src_count);

#endif /* LEUKO_UTIL_STRING_ARRAY_H */
