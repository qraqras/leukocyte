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
    /* Optional multi-document-aware apply function: docs ordered parent-first */
    bool (*apply_yaml_multi)(rule_config_t *config, yaml_document_t **docs, size_t doc_count, const char *full_name, const char *category_name, char **err);
};

typedef struct config_ops config_ops_t;

#endif /* LEUKOCYTE_CONFIGS_CONFIG_OPS_H */
