// Test that generated config struct is initialized and freed correctly
#include <stdlib.h>
#include <assert.h>

#include "configs/config.h"

int main(void)
{
    leuko_config_t cfg;
    leuko_config_initialize(&cfg);

    // At least the indentation rule should be initialized
#define X(field, cat_name, sname, rule_ptr, ops_ptr) assert(cfg.field != NULL);
    LEUKO_RULES_LIST
#undef X

    leuko_config_free(&cfg);

    return 0;
}
