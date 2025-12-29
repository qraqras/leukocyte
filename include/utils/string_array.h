#ifndef LEUKO_UTIL_STRING_ARRAY_H
#define LEUKO_UTIL_STRING_ARRAY_H

#include <stdbool.h>
#include <stddef.h>

bool leuko_str_arr_push(char ***dest, size_t *dest_count, const char *s);
bool leuko_str_arr_concat(char ***dest, size_t *dest_count, char **src, size_t src_count);
bool leuko_str_arr_split(const char *str, const char *delimiter, char ***out, size_t *out_count);

#endif /* LEUKO_UTIL_STRING_ARRAY_H */
