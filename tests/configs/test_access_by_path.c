#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "configs/compiled_config.h"
#include "common/rule_registry.h"
#include "configs/common/rule_config.h"
/* Forward-declare minimal types and needed prototypes to avoid pulling heavy headers (prism etc.) */
typedef struct leuko_config_s leuko_config_t;
typedef struct leuko_config_rule_base_s leuko_config_rule_base_t;

/* Minimal prototype for helper used by the test */
leuko_config_rule_base_t *leuko_config_get_rule(leuko_config_t *cfg, const char *category, const char *rule_name);

static void write_file(const char *path, const char *content)
{
    FILE *f = fopen(path, "w");
    assert(f);
    fputs(content, f);
    fclose(f);
}

int main(void)
{
    const char *d = "tests/tmp_access_path";
    mkdir(d, 0755);
    char path[1024];
    snprintf(path, sizeof(path), "%s/.rubocop.yml", d);
    const char *json = "{ \"categories\": { \"Layout\": { \"rules\": { \"IndentationConsistency\": { \"Include\": [\"**/*.rb\"] } } } } }\n";
    write_file(path, json);

    leuko_compiled_config_t *cfg = leuko_compiled_config_build(d, NULL);
    assert(cfg);
    const leuko_config_t *ef = leuko_compiled_config_rules(cfg);
    assert(ef);

    leuko_config_rule_base_t *rconf = leuko_config_get_rule((leuko_config_t *)ef, "Layout", "IndentationConsistency");
    assert(rconf != NULL);
    assert(rconf->include_count == 1);
    assert(strcmp(rconf->include[0], "**/*.rb") == 0);

    /* Static view access (via compiled config accessor) */
    leuko_config_rule_view_t *rv = leuko_compiled_config_view_rule(cfg, "Layout", "IndentationConsistency");
    assert(rv != NULL);
    assert(rv->base.include_count == 1);
    assert(strcmp(rv->base.include[0], "**/*.rb") == 0);

    leuko_compiled_config_unref(cfg);
    remove(path);
    rmdir(d);

    printf("OK\n");
    return 0;
}
