#ifndef LEUKOCYTE_IO_SCAN_H
#define LEUKOCYTE_IO_SCAN_H

#include <stddef.h>
#include <stdbool.h>

bool leuko_collect_ruby_files(char **paths, size_t paths_count, char ***out_paths, size_t *out_count, char **err);
bool leuko_collect_ruby_files_with_exclude(char **paths, size_t paths_count, char ***out_paths, size_t *out_count, char **excludes, size_t exclude_count, char **err);

#endif /* LEUKOCYTE_IO_SCAN_H */
