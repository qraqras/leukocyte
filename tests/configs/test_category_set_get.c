#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>
#include "configs/common/category_config.h"
#include "configs/common/rule_config.h"

/* Minimal opaque declarations to avoid pulling heavy headers into the test
 * (we only need an allocated config pointer and the set/get helpers). */
typedef struct leuko_config_s leuko_config_t;
leuko_config_t *leuko_config_new(void);
void leuko_config_free(leuko_config_t *cfg);
void leuko_config_set_view_rule(leuko_config_t *cfg, const char *category, const char *rule_name, leuko_config_rule_view_t *rconf);
leuko_config_rule_base_t *leuko_config_get_rule(leuko_config_t *cfg, const char *category, const char *rule_name);

int main(void)
{
    leuko_config_t *cfg = leuko_config_new();
    if (!cfg)
        return 1;

    leuko_config_rule_view_t *r1 = leuko_rule_config_initialize();
    assert(r1 != NULL);
    r1->base.severity = LEUKO_SEVERITY_ERROR; /* mark with non-default to detect transfer */
    leuko_config_set_view_rule(cfg, "Layout", "IndentationConsistency", r1);
    leuko_config_rule_base_t *gr1 = leuko_config_get_rule(cfg, "Layout", "IndentationConsistency");
    assert(gr1 != NULL);
    assert(gr1->severity == LEUKO_SEVERITY_ERROR);

    leuko_config_rule_view_t *r2 = leuko_rule_config_initialize();
    assert(r2 != NULL);
    r2->base.severity = LEUKO_SEVERITY_WARNING;
    leuko_config_set_view_rule(cfg, "Layout", "IndentationWidth", r2);
    leuko_config_rule_base_t *gr2 = leuko_config_get_rule(cfg, "Layout", "IndentationWidth");
    assert(gr2 != NULL);
    assert(gr2->severity == LEUKO_SEVERITY_WARNING);

    /* cleanup */
    leuko_config_free(cfg);

    printf("OK\n");
    return 0;
}
