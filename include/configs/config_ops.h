/* Operations for rule config: bundled create_default and apply_yaml */
#ifndef LEUKOCYTE_CONFIGS_CONFIG_OPS_H
#define LEUKOCYTE_CONFIGS_CONFIG_OPS_H

#include <yaml.h>
#include "configs/config.h"
#include "prism.h"

struct config_ops
{
    rule_config_t *(*initialize)(void);
    bool (*apply_yaml)(rule_config_t *config, const yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, char **err);
};

typedef struct config_ops config_ops_t;

#endif /* LEUKOCYTE_CONFIGS_CONFIG_OPS_H */
