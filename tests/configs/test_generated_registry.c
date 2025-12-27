#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "common/rule_registry.h"

int main(void)
{
    const leuko_registry_category_t *cats = leuko_get_rule_categories();
    size_t n = leuko_get_rule_category_count();
    assert(n >= 1);
    /* find Layout category */
    const leuko_registry_category_t *c = NULL;
    for (size_t i = 0; i < n; i++)
    {
        if (cats[i].name && strcmp(cats[i].name, "Layout") == 0)
        {
            c = &cats[i];
            break;
        }
    }
    assert(c != NULL);
    assert(c->count >= 1);
    assert(strcmp(c->entries[0].name, "IndentationConsistency") == 0);
    printf("OK\n");
    return 0;
}
