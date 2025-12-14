/* Very small placeholder YAML reader wrapper. Not implemented; returns NULLs.
 */
#include <stdlib.h>
#include <string.h>
#include "configs/yaml_reader.h"

struct yaml_reader_s
{
    char *path;
};

yaml_reader_t *yaml_reader_create_from_file(const char *path, char **err)
{
    if (!path)
    {
        if (err)
            *err = strdup("path is NULL");
        return NULL;
    }
    yaml_reader_t *r = calloc(1, sizeof(yaml_reader_t));
    if (!r)
    {
        if (err)
            *err = strdup("alloc failure");
        return NULL;
    }
    r->path = strdup(path);
    return r;
}

void yaml_reader_free(yaml_reader_t *r)
{
    if (!r)
        return;
    free(r->path);
    free(r);
}

bool yaml_reader_has_key(yaml_reader_t *r, const char *key)
{
    (void)r;
    (void)key;
    return false;
}

const char *yaml_reader_get_string(yaml_reader_t *r, const char *key)
{
    (void)r;
    (void)key;
    return NULL;
}
