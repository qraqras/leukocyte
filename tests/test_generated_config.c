// Test that generated config struct is initialized and freed correctly
#include <stdlib.h>
#include <assert.h>

#include "configs/generated_config.h"

int main(void)
{
    config_t cfg;
    initialize_config(&cfg);

    // At least the indentation rule should be initialized
#define X(field, cat_name, sname, rule_ptr, ops_ptr) assert(cfg.field != NULL);
    LEUKO_RULES_LIST
#undef X

    free_config(&cfg);

    return 0;
}
