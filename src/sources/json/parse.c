#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <cJSON.h>
#include "sources/json/parse.h"
#include <errno.h>
#include "sources/node.h"

/**
 * @brief Internal helper to convert cJSON structure to leuko_node_t.
 * @param cjson Pointer to the cJSON structure
 * @return Pointer to the resulting structure
 */
static leuko_node_t *leuko_node_from_cjson(const cJSON *cjson)
{
    if (!cjson)
    {
        return NULL;
    }
    int type = cjson->type & 0xff;
    switch (type)
    {
    case cJSON_Object:
    {
        size_t n = (size_t)cJSON_GetArraySize(cjson);
        leuko_node_t *obj = calloc(1, sizeof(leuko_node_t));
        if (!obj)
        {
            return NULL;
        }
        obj->type = LEUKO_NODE_OBJECT;
        obj->map_len = n;
        obj->map_keys = n ? calloc(n, sizeof(char *)) : NULL;
        obj->map_vals = n ? calloc(n, sizeof(leuko_node_t *)) : NULL;
        size_t i = 0;
        for (const cJSON *c = cjson->child; c; c = c->next, i++)
        {
            obj->map_keys[i] = c->string ? strdup(c->string) : NULL;
            obj->map_vals[i] = leuko_node_from_cjson(c);
        }
        return obj;
    }
    case cJSON_Array:
    {
        size_t n = (size_t)cJSON_GetArraySize(cjson);
        leuko_node_t *arr = calloc(1, sizeof(leuko_node_t));
        if (!arr)
        {
            return NULL;
        }
        arr->type = LEUKO_NODE_ARRAY;
        arr->arr_len = n;
        arr->map_vals = n ? calloc(n, sizeof(leuko_node_t *)) : NULL;
        size_t i = 0;
        for (const cJSON *c = cjson->child; c; c = c->next, i++)
        {
            arr->map_vals[i] = leuko_node_from_cjson(c);
        }
        return arr;
    }
    case cJSON_String:
    {
        return leuko_node_new_scalar(LEUKO_NODE_STRING, cjson->valuestring);
    }
    case cJSON_True:
    {
        return leuko_node_new_scalar(LEUKO_NODE_BOOLEAN, "true");
    }
    case cJSON_False:
    {
        return leuko_node_new_scalar(LEUKO_NODE_BOOLEAN, "false");
    }
    case cJSON_Number:
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "%.17g", cjson->valuedouble);
        return leuko_node_new_scalar(LEUKO_NODE_NUMBER, buf);
    }
    case cJSON_NULL:
    {
        leuko_node_t *n = calloc(1, sizeof(leuko_node_t));
        if (!n)
        {
            return NULL;
        }
        n->type = LEUKO_NODE_NULL;
        return n;
    }
    default:
    {
        return NULL;
    }
    }
}

/**
 * @brief Parse JSON from buffer into leuko_node_t structure.
 * @param buf Pointer to the buffer containing JSON data
 * @param len Length of the buffer
 * @param out Pointer to store the resulting leuko_node_t structure
 * @return true on success, false on failure
 */
static bool leuko_json_parse_from_buffer(const char *buf, size_t len, leuko_node_t **out)
{
    if (!buf || !out)
    {
        errno = EINVAL;
        return false;
    }
    if ((int)len <= 0)
    {
        errno = EINVAL;
        return false;
    }
    cJSON *root = cJSON_ParseWithLength(buf, len);
    if (!root)
    {
        errno = EILSEQ;
        return false;
    }
    leuko_node_t *node = leuko_node_from_cjson(root);
    cJSON_Delete(root);
    if (!node)
    {
        errno = EILSEQ;
        return false;
    }
    *out = node;
    return true;
}

/**
 * @brief Parse JSON file into leuko_node_t structure.
 * @param path Path to the JSON file
 * @param out Pointer to store the resulting leuko_node_t structure
 * @return true on success, false on failure
 */
bool leuko_json_parse(const char *path, leuko_node_t **out)
{
    if (!path || !out)
    {
        errno = EINVAL;
        return false;
    }
    FILE *f = fopen(path, "rb");
    if (!f)
    {
        /* fopen sets errno */
        return false;
    }
    if (fseek(f, 0, SEEK_END) != 0)
    {
        errno = EIO;
        fclose(f);
        return false;
    }
    long sz = ftell(f);
    if (sz < 0)
    {
        errno = EIO;
        fclose(f);
        return false;
    }
    rewind(f);
    char *buf = malloc((size_t)sz + 1);
    if (!buf)
    {
        errno = ENOMEM;
        fclose(f);
        return false;
    }
    size_t n = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    if (n != (size_t)sz)
    {
        free(buf);
        errno = EILSEQ;
        return false;
    }
    bool ok = leuko_json_parse_from_buffer(buf, n, out);
    free(buf);
    return ok;
}
