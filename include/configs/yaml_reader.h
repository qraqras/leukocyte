/* Minimal YAML reader wrapper API (skeleton)
 * This will wrap libyaml calls and provide helpers to query keys.
 */
#ifndef LEUKOCYTE_YAML_READER_H
#define LEUKOCYTE_YAML_READER_H

#include <stdbool.h>

typedef struct yaml_reader_s yaml_reader_t;

yaml_reader_t *yaml_reader_create_from_file(const char *path, char **err);
void yaml_reader_free(yaml_reader_t *r);

bool yaml_reader_has_key(yaml_reader_t *r, const char *key);
const char *yaml_reader_get_string(yaml_reader_t *r, const char *key);

#endif // LEUKOCYTE_YAML_READER_H
