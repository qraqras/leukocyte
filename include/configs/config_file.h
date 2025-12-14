#ifndef LEUKOCYTE_CONFIG_FILE_H
#define LEUKOCYTE_CONFIG_FILE_H

#include <stddef.h>
#include <stdbool.h>
#include "configs/config.h"

// Forward declare typed allocations; each rule provides its typed struct.
// Example: typedef struct indentation_consistency_s indentation_consistency_t;

// Common rule config (auto-generated per rule as part of file_t)
typedef struct
{
    bool enabled;
    int severity;
    const char **include;
    size_t include_count;
    const char **exclude;
    size_t exclude_count;
} common_config_t;

// Forward declare typed types used here
typedef struct indentation_consistency_s indentation_consistency_t;

// Define file_t manually (no macros). Add one field per rule: a common
// config and a typed pointer for rule-specific options.
struct file_t
{
    common_config_t IndentationConsistency_common;
    indentation_consistency_t *IndentationConsistency;
};

// Provide typedef for convenience
typedef struct file_t file_t;

// API
file_t *file_clone(const file_t *src);
void file_free(file_t *cfg);
bool config_file_apply_yaml(file_t *cfg, const char *yaml_path, char **err);

// Per-rule apply prototype (implemented manually)
void apply_yaml_to_IndentationConsistency(const char *yaml_path, common_config_t *common, indentation_consistency_t **typed_out);

#endif // LEUKOCYTE_CONFIG_FILE_H
