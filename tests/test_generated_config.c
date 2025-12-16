// Test that generated config struct is initialized and freed correctly
#include <stdlib.h>
#include <assert.h>

#include "configs/generated_config.h"

int main(void)
{
    config_t cfg;
    config_initialize(&cfg);

    // At least the indentation rule should be initialized
#define X(field, cat, name, rule_ptr, ops_ptr) assert(cfg.field != NULL);
    RULES_LIST
#undef X

    config_free(&cfg);

    return 0;
}
