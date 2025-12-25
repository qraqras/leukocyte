#ifndef LEUKOCYTE_CONFIGS_ALL_COPS_CONFIG_H
#define LEUKOCYTE_CONFIGS_ALL_COPS_CONFIG_H

#include "configs/common/general_config.h"

/* Backwards compatibility shim: AllCops -> general */
typedef leuko_general_config_t leuko_all_cops_config_t;

static inline leuko_all_cops_config_t *leuko_config_get_all_cops_config(leuko_config_t *cfg)
{
    return leuko_config_get_general_config(cfg);
}

#endif /* LEUKOCYTE_CONFIGS_ALL_COPS_CONFIG_H */
