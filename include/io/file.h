#ifndef LEUKOCYTE_IO_FILE_H
#define LEUKOCYTE_IO_FILE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/* Read the contents of `path` into a newly allocated buffer.
 * On success: returns true, sets *out_buf (malloc'd) and *out_size.
 * On failure: returns false and if `err` is provided, sets *err to a strdup'd message.
 * Caller is responsible for freeing *out_buf and free(*err) if set.
 */
bool read_file_to_buffer(const char *path, uint8_t **out_buf, size_t *out_size, char **err);

#endif /* LEUKOCYTE_IO_FILE_H */
