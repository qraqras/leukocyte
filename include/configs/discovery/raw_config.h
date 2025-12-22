#ifndef LEUKOCYTE_CONFIGS_RAW_CONFIG_H
#define LEUKOCYTE_CONFIGS_RAW_CONFIG_H

#include <yaml.h>

/* Raw parsed YAML config wrapper */
typedef struct leuko_raw_config_s
{
    yaml_document_t *doc; /* owned */
    char *path;           /* config file path (owned) */
    size_t refcount;      /* reference count for shared ownership */
} leuko_raw_config_t;

/* Load a YAML config file. Returns 0 on success, non-zero on error and sets *err to a short message (caller frees). */
int leuko_config_load_file(const char *path, leuko_raw_config_t **out, char **err);

/* Manage reference counting for raw config objects */
void leuko_raw_config_ref(leuko_raw_config_t *cfg);
void leuko_raw_config_unref(leuko_raw_config_t *cfg);

/* Free a raw config (alias to unref for compatibility) */
void leuko_raw_config_free(leuko_raw_config_t *cfg);

#endif /* LEUKOCYTE_CONFIGS_RAW_CONFIG_H */
