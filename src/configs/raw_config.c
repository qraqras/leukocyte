/*
 * Raw YAML config loader
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <yaml.h>

#include "configs/raw_config.h"

/* Read entire file into buffer (caller frees buffer) */
static char *read_file_to_buffer(const char *path, size_t *out_len)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return NULL;
    if (fseek(f, 0, SEEK_END) != 0)
    {
        fclose(f);
        return NULL;
    }
    long sz = ftell(f);
    if (sz < 0)
    {
        fclose(f);
        return NULL;
    }
    if (fseek(f, 0, SEEK_SET) != 0)
    {
        fclose(f);
        return NULL;
    }
    char *buf = (char *)malloc((size_t)sz + 1);
    if (!buf)
    {
        fclose(f);
        return NULL;
    }
    size_t n = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[n] = '\0';
    if (out_len)
        *out_len = n;
    return buf;
}

int leuko_config_load_file(const char *path, leuko_raw_config_t **out, char **err)
{
    if (!path || !out)
    {
        if (err)
            *err = strdup("invalid arguments");
        return 1;
    }

    size_t len = 0;
    char *buf = read_file_to_buffer(path, &len);
    if (!buf)
    {
        if (err)
            *err = strdup("could not read config file");
        return 1;
    }

    yaml_parser_t parser;
    yaml_document_t *doc = (yaml_document_t *)malloc(sizeof(yaml_document_t));
    if (!doc)
    {
        free(buf);
        if (err)
            *err = strdup("allocation failure");
        return 1;
    }

    yaml_parser_initialize(&parser);
    yaml_parser_set_input_string(&parser, (const unsigned char *)buf, (size_t)len);

    if (!yaml_parser_load(&parser, doc))
    {
        /* short error message per project policy */
        yaml_parser_delete(&parser);
        free(doc);
        free(buf);
        if (err)
            *err = strdup("failed to parse YAML config");
        return 1;
    }

    yaml_parser_delete(&parser);
    free(buf);

    leuko_raw_config_t *cfg = (leuko_raw_config_t *)malloc(sizeof(leuko_raw_config_t));
    if (!cfg)
    {
        yaml_document_delete(doc);
        free(doc);
        if (err)
            *err = strdup("allocation failure");
        return 1;
    }

    cfg->doc = doc;
    cfg->path = strdup(path);
    cfg->refcount = 1; /* initial ownership */
    *out = cfg;
    return 0;
}

void leuko_raw_config_ref(leuko_raw_config_t *cfg)
{
    if (!cfg)
        return;
    cfg->refcount += 1;
}

void leuko_raw_config_unref(leuko_raw_config_t *cfg)
{
    if (!cfg)
        return;
    if (cfg->refcount > 1)
    {
        cfg->refcount -= 1;
        return;
    }
    /* refcount == 1 -> free */
    if (cfg->doc)
    {
        yaml_document_delete(cfg->doc);
        free(cfg->doc);
        cfg->doc = NULL;
    }
    if (cfg->path)
    {
        free(cfg->path);
        cfg->path = NULL;
    }
    free(cfg);
}

void leuko_raw_config_free(leuko_raw_config_t *cfg)
{
    /* Backwards-compatible alias */
    leuko_raw_config_unref(cfg);
}
