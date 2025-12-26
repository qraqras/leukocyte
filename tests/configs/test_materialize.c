#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "configs/compiled_config.h"
#include "common/rule_registry.h"
#include "configs/common/rule_config.h"

/* Forward-declare minimal types to avoid pulling heavy headers (prism etc.) */
typedef struct leuko_config_s leuko_config_t;
typedef struct leuko_rule_config_s leuko_rule_config_t;
leuko_rule_config_t *leuko_rule_config_get_by_index(leuko_config_t *cfg, size_t idx);
static void write_file(const char *path, const char *content)
{
    FILE *f = fopen(path, "w");
    assert(f);
    fputs(content, f);
    fclose(f);
}

int main(void)
{
    /* create temp dir */
    const char *d = "tests/tmp_matdir";
    mkdir(d, 0755);
    char path[1024];
    snprintf(path, sizeof(path), "%s/.rubocop.yml", d);
    const char *json = "{ \"Layout\": { \"IndentationConsistency\": { \"Include\": [\"**/*.rb\"] } } }\n";
    write_file(path, json);

    leuko_compiled_config_t *cfg = leuko_compiled_config_build(d, NULL);
    assert(cfg);

    /* The rule include should be applied to the IndentationConsistency rule */
    const leuko_config_t *ef = leuko_compiled_config_rules(cfg);
    assert(ef);
    size_t idx = leuko_rule_find_index("Layout", "IndentationConsistency");
    assert(idx != SIZE_MAX);
    leuko_rule_config_t *rconf = leuko_rule_config_get_by_index((leuko_config_t *)ef, idx);
    assert(rconf != NULL);
    assert(rconf->include_count == 1);
    assert(strcmp(rconf->include[0], "**/*.rb") == 0);

    /* cleanup */
    leuko_compiled_config_unref(cfg);
    remove(path);
    rmdir(d);

    printf("OK\n");
    return 0;
}
