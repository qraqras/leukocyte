/* Test that pm_diagnostic_list_free correctly frees owned messages */
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "prism/diagnostic.h"

extern size_t g_diag_freed;

int main(void)
{
    pm_list_t list = {0};
    size_t N = 16;
    for (size_t i = 0; i < N; i++)
    {
        pm_diagnostic_t *d = calloc(1, sizeof(pm_diagnostic_t));
        assert(d != NULL);
        d->node.next = NULL;
        d->message = strdup("test");
        d->owned = true;
        d->level = PM_ERROR_LEVEL_SYNTAX;
        pm_list_append(&list, (pm_list_node_t *)d);
    }

    /* Reset counter then free list */
    g_diag_freed = 0;
    pm_diagnostic_list_free(&list);
    if (g_diag_freed != N)
    {
        fprintf(stderr, "expected g_diag_freed=%zu got=%zu\n", N, g_diag_freed);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
