#ifndef LEUKOCYTE_IO_FILE_H
#define LEUKOCYTE_IO_FILE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

bool read_file_to_buffer(const char *path, uint8_t **out_buf, size_t *out_size, char **err);

#endif /* LEUKOCYTE_IO_FILE_H */
