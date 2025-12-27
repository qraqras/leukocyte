#include <assert.h>
#include <stdio.h>

#include "configs/compiled_config.h"

/* Minimal forward declarations to avoid pulling heavy rule headers */
typedef struct leuko_config_rule_base_s leuko_config_rule_base_t;

/* Minimal view layout matching the generated view (only the field we assert) */
typedef struct leuko_test_categories_s
{
    struct
    {
        struct
        {
            leuko_config_rule_base_t *indentation_consistency;
        } rules;
    } layout;
} leuko_test_categories_view_t;

/* Minimal leuko_config_t shape (we only need categories for this test) */
typedef struct leuko_test_config_s
{
    leuko_test_categories_view_t categories;
} leuko_test_config_t;

/* Minimal prototype for helper used by the test (use void* to avoid strict type conflict) */
leuko_config_rule_base_t *leuko_config_get_rule(void *cfg, const char *category, const char *rule_name);

/* Ensure categories pointers match category/rule lookups */
int main(void)
{
    const char *d = "tests/tmp_parity";
    mkdir(d, 0755);
    char path[1024];
    snprintf(path, sizeof(path), "%s/.rubocop.yml", d);
    const char *json = "{ \"categories\": { \"Layout\": { \"rules\": { \"IndentationConsistency\": { \"Include\": [\"**/*.rb\"] }, \"IndentationWidth\": { \"Include\": [\"**/*.rb\"] }, \"IndentationStyle\": { \"Include\": [\"**/*.rb\"] }, \"LineLength\": { \"Include\": [\"**/*.rb\"] } } } } }\n";
    FILE *f = fopen(path, "w");
    assert(f);
    fputs(json, f);
    fclose(f);

    leuko_compiled_config_t *cfg = leuko_compiled_config_build(d, NULL);
    assert(cfg);
    const leuko_config_t *ef = leuko_compiled_config_rules(cfg);
    assert(ef);

    /* For each rule in layout ensure view and API point to same object */
    const char *rules[] = {"IndentationConsistency", "IndentationWidth", "IndentationStyle", "LineLength"};
    for (size_t i = 0; i < sizeof(rules) / sizeof(*rules); i++)
    {
        const char *rn = rules[i];
        leuko_config_rule_base_t *r_api = leuko_config_get_rule((leuko_config_t *)ef, "Layout", rn);
        assert(r_api);
        leuko_config_rule_base_t *r_view = leuko_compiled_config_view_rule(cfg, "Layout", rn);
        assert(r_view);
        assert(r_api == r_view);
        assert(r_api->include_count == 1);
    }

    leuko_compiled_config_unref(cfg);
    remove(path);
    rmdir(d);

    printf("OK\n");
    return 0;
}
