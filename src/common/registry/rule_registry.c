#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "common/registry/registry.h"
#include "common/generated_rules.h"

/* Compatibility shim: construct a flat rule_registry_entry_t array from the
 * category-indexed generated registry. This preserves the old API while the
 * rest of the code migrates to the new generated structure.
 */

const rule_registry_entry_t *leuko_get_rule_registry(void)
{
    static rule_registry_entry_t *cache = NULL;
    static size_t cache_count = 0;
    if (cache)
        return cache;

    const leuko_rule_category_registry_t *cats = leuko_get_rule_categories();
    size_t cats_n = leuko_get_rule_category_count();

    /* count total entries */
    size_t total = 0;
    for (size_t i = 0; i < cats_n; i++)
        total += cats[i].count;
    if (total == 0)
    {
        cache = NULL;
        cache_count = 0;
        return cache;
    }

    rule_registry_entry_t *arr = calloc(total, sizeof(rule_registry_entry_t));
    if (!arr)
        return NULL;

    /* Keep allocated full_name strings in a separate array to own them */
    char **full_names = calloc(total, sizeof(char *));
    if (!full_names)
    {
        free(arr);
        return NULL;
    }

    size_t idx = 0;
    for (size_t ci = 0; ci < cats_n; ci++)
    {
        const leuko_rule_category_registry_t *cat = &cats[ci];
        for (size_t ei = 0; ei < cat->count; ei++)
        {
            const leuko_rule_registry_entry_t *ent = &cat->entries[ei];
            arr[idx].category_name = cat->category;
            arr[idx].rule_name = ent->name;
            /* build full name "Category/Name" */
            size_t len = strlen(cat->category) + 1 + strlen(ent->name) + 1;
            char *fn = malloc(len);
            if (fn)
            {
                snprintf(fn, len, "%s/%s", cat->category, ent->name);
                full_names[idx] = fn;
                arr[idx].full_name = full_names[idx];
            }
            else
            {
                arr[idx].full_name = NULL;
            }
            /* map the opaque rule pointer if present */
            arr[idx].rule = (rule_t *)ent->rule;
            arr[idx].handlers = ent->handlers;
            idx++;
        }
    }

    /* stash cache: keep full_names and arr allocated for process lifetime */
    cache = arr;
    cache_count = total;
    return cache;
}

size_t leuko_get_rule_registry_count(void)
{
    const leuko_rule_category_registry_t *cats = leuko_get_rule_categories();
    size_t cats_n = leuko_get_rule_category_count();
    size_t total = 0;
    for (size_t i = 0; i < cats_n; i++)
        total += cats[i].count;
    return total;
}
