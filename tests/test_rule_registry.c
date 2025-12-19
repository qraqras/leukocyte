#include <assert.h>
#include <string.h>

#include "rule_registry.h"

int main(void)
{
    const rule_registry_entry_t *reg = leuko_get_rule_registry();
    size_t count = leuko_get_rule_registry_count();

    int found = 0;
    for (size_t i = 0; i < count; i++)
    {
        if (strcmp(reg[i].category_name, "Layout") == 0 && strcmp(reg[i].rule_name, "IndentationConsistency") == 0)
        {
            assert(strcmp(reg[i].full_name, "Layout/IndentationConsistency") == 0);
            found = 1;
            break;
        }
    }
    assert(found && "Expected Layout/IndentationConsistency entry in registry");
    return 0;
}
