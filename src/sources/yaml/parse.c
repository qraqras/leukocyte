#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "sources/yaml/parse.h"
#include "leuko_debug.h"

bool leuko_yaml_parse(const char *path, yaml_document_t **out_doc)
{
    if (!path || !out_doc)
    {
        return false;
    }

    FILE *fh = fopen(path, "rb");
    if (!fh)
    {
        LDEBUG("yaml: failed to open %s: %s", path, strerror(errno));
        return false;
    }

    yaml_parser_t parser;
    if (!yaml_parser_initialize(&parser))
    {
        fclose(fh);
        LDEBUG("yaml: parser init failed for %s", path);
        return false;
    }
    yaml_parser_set_input_file(&parser, fh);

    yaml_document_t *doc = malloc(sizeof(yaml_document_t));
    if (!doc)
    {
        yaml_parser_delete(&parser);
        fclose(fh);
        LDEBUG("yaml: out of memory allocating document for %s", path);
        return false;
    }

    if (!yaml_parser_load(&parser, doc))
    {
        yaml_document_delete(doc);
        free(doc);
        yaml_parser_delete(&parser);
        fclose(fh);
        LDEBUG("yaml: parse failed for %s", path);
        return false;
    }

    /* success */
    yaml_parser_delete(&parser);
    fclose(fh);
    *out_doc = doc;
    return true;
}
