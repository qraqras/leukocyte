#ifndef LEUKO_CONFIGS_WALKER_H
#define LEUKO_CONFIGS_WALKER_H

#include <stddef.h>
#include <stdbool.h>

#include "configs/compiled_config.h"

typedef struct leuko_collected_file_s
{
    char *path;                      /* 絶対または相対パス（所有: 呼び出し側が free） */
    leuko_compiled_config_t *config; /* 対応する compiled-config（ref を持ちます） */
} leuko_collected_file_t;

/* Collect Ruby files under start_dir. Returns 0 on success, negative on error. */
int leuko_config_walker_collect(const char *start_dir,
                                leuko_collected_file_t **out_files,
                                size_t *out_count);

/* Free collected files (unref configs and free path strings) */
void leuko_collected_files_free(leuko_collected_file_t *files, size_t count);

#endif /* LEUKO_CONFIGS_WALKER_H */
