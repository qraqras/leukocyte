#include "configs/diagnostics.h"
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

typedef struct config_diagnostic_node_s
{
    pm_list_node_t node;
    char *msg;
} config_diagnostic_node_t;

void config_diagnostics_append(pm_list_t *list, int line, int column, const char *fmt, ...)
{
    if (!list || !fmt)
        return;

    va_list ap;
    va_start(ap, fmt);
    char buf[512];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    config_diagnostic_node_t *n = malloc(sizeof(*n));
    if (!n)
        return;
    n->msg = NULL;
    if (line >= 0)
    {
        char buf2[640];
        if (column >= 0)
            snprintf(buf2, sizeof(buf2), "line %d col %d: %s", line, column, buf);
        else
            snprintf(buf2, sizeof(buf2), "line %d: %s", line, buf);
        n->msg = strdup(buf2);
    }
    else
    {
        n->msg = strdup(buf);
    }
    n->node.next = NULL;
    pm_list_append(list, &n->node);
}

const char *config_diagnostics_first_message(pm_list_t *list)
{
    if (!list || !list->head)
        return NULL;
    config_diagnostic_node_t *n = (config_diagnostic_node_t *)list->head;
    return n->msg;
}

void config_diagnostics_print_all(FILE *out, pm_list_t *list)
{
    if (!out || !list)
        return;
    pm_list_node_t *node = list->head;
    while (node)
    {
        config_diagnostic_node_t *dn = (config_diagnostic_node_t *)node;
        if (dn->msg)
            fprintf(out, "Config diagnostic: %s\n", dn->msg);
        node = node->next;
    }
}

void config_diagnostics_free(pm_list_t *list)
{
    if (!list)
        return;
    pm_list_node_t *node = list->head;
    while (node)
    {
        config_diagnostic_node_t *dn = (config_diagnostic_node_t *)node;
        if (dn->msg)
            free(dn->msg);
        node = node->next;
    }
    // free nodes
    pm_list_free(list);
}
