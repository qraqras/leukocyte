#include "configs/schema/validator.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "sources/node/node_stub.c" /* reuse helpers (ok for prototype) */

static char *dupstr(const char *s)
{
    return s ? strdup(s) : NULL;
}

void leuko_schema_diag_free(leuko_schema_diag_t *d)
{
    if (!d)
        return;
    free(d->file);
    free(d->path);
    free(d->message);
    free(d);
}

static void append_diag(leuko_schema_diag_t ***diags, size_t *count, const char *file, const char *path, const char *msg)
{
    leuko_schema_diag_t *d = calloc(1, sizeof(*d));
    d->file = dupstr(file);
    d->path = dupstr(path);
    d->message = dupstr(msg);
    *diags = realloc(*diags, (*count + 1) * sizeof(**diags));
    (*diags)[*count] = d;
    (*count)++;
}

bool leuko_schema_validate_layout(const leuko_node_t *merged, leuko_schema_diag_t ***out_diags, size_t *out_count)
{
    if (!merged || !out_diags || !out_count)
        return false;
    *out_diags = NULL;
    *out_count = 0;

    /* Find categories.Layout.rules */
    const leuko_node_t *categories = leuko_node_get_mapping_child(merged, "categories");
    if (!categories || categories->type != LEUKO_NODE_OBJECT)
        return true; /* nothing to validate */

    const leuko_node_t *layout = leuko_node_get_mapping_child(categories, "Layout");
    if (!layout || layout->type != LEUKO_NODE_OBJECT)
        return true;

    const leuko_node_t *rules = leuko_node_get_mapping_child(layout, "rules");
    if (!rules || rules->type != LEUKO_NODE_OBJECT)
        return true;

    /* IndentationWidth.Width -> must be integer >= 0 */
    const leuko_node_t *iw = leuko_node_get_mapping_child(rules, "IndentationWidth");
    if (iw && iw->type == LEUKO_NODE_OBJECT)
    {
        const leuko_node_t *width = leuko_node_get_mapping_child(iw, "Width");
        if (!width || !LEUKO_NODE_IS_SCALAR(width->type))
        {
            append_diag(out_diags, out_count, NULL, "Layout/IndentationWidth/Width", "Width must be an integer");
        }
        else
        {
            char *end = NULL;
            long v = strtol(width->scalar ?: "", &end, 10);
            if (!width->scalar || *end != '\0')
            {
                append_diag(out_diags, out_count, NULL, "Layout/IndentationWidth/Width", "Width must be an integer");
            }
            else if (v < 0)
            {
                append_diag(out_diags, out_count, NULL, "Layout/IndentationWidth/Width", "Width must be >= 0");
            }
        }
    }

    /* LineLength.Max -> integer >= 0 */
    const leuko_node_t *ll = leuko_node_get_mapping_child(rules, "LineLength");
    if (ll && ll->type == LEUKO_NODE_OBJECT)
    {
        const leuko_node_t *max = leuko_node_get_mapping_child(ll, "Max");
        if (!max || !LEUKO_NODE_IS_SCALAR(max->type))
        {
            append_diag(out_diags, out_count, NULL, "Layout/LineLength/Max", "Max must be an integer");
        }
        else
        {
            char *end = NULL;
            long v = strtol(max->scalar ?: "", &end, 10);
            if (!max->scalar || *end != '\0')
            {
                append_diag(out_diags, out_count, NULL, "Layout/LineLength/Max", "Max must be an integer");
            }
            else if (v < 0)
            {
                append_diag(out_diags, out_count, NULL, "Layout/LineLength/Max", "Max must be >= 0");
            }
        }
    }

    return (*out_count == 0);
}
