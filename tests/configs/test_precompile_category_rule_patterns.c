#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "configs/compiled_config.h"
#include "common/registry/registry.h"
#include "configs/common/category_config.h"
#include "configs/common/rule_config.h"
#include "configs/walker.h"

int main(void)
{
    const char *td = "tests/tmp_precompile2";
    system("rm -rf tests/tmp_precompile2 && mkdir -p tests/tmp_precompile2/vendor && mkdir -p tests/tmp_precompile2/lib && touch tests/tmp_precompile2/lib/d.rb tests/tmp_precompile2/a.rb tests/tmp_precompile2/vendor/b.rb");
    FILE *f = fopen("tests/tmp_precompile2/.rubocop.yml", "w");
    if (!f)
        return 2;
    // Category-level exclude lib/**/* and a rule-level include only rb
    fprintf(f, "Layout:\n  Exclude:\n    - \"lib/**/*\"\nLayout/IndentationConsistency:\n  Include:\n    - \"**/*.rb\"\n  Exclude:\n    - \"vendor/**/*\"\n");
    fclose(f);

    leuko_compiled_config_t *c = leuko_compiled_config_build(td, NULL);
    if (!c)
    {
        fprintf(stderr, "build failed\n");
        return 2;
    }

    const leuko_category_config_t *cc = leuko_compiled_config_get_category(c, "Layout");
    if (!cc)
    {
        fprintf(stderr, "no category\n");
        leuko_compiled_config_unref(c);
        return 2;
    }
    if (cc->exclude_re_count == 0)
    {
        fprintf(stderr, "category exclude not compiled\n");
        leuko_compiled_config_unref(c);
        return 2;
    }

    /* check rule-level regex compiled */
    size_t idx = 0; /* find a rule config with compiled patterns */
    leuko_config_t *cfg = (leuko_config_t *)leuko_compiled_config_rules(c);
    /* prototype to avoid pulling heavy headers */
    extern leuko_rule_config_t *leuko_rule_config_get_by_index(leuko_config_t * cfg, size_t idx);
    size_t max_idx = leuko_get_rule_registry_count();
    leuko_rule_config_t *rconf = leuko_rule_config_get_by_index(cfg, 0);
    while (rconf && rconf->include_re_count == 0 && rconf->exclude_re_count == 0 && idx < max_idx)
    {
        idx++;
        rconf = leuko_rule_config_get_by_index(cfg, idx);
    }
    if (!rconf)
    {
        fprintf(stderr, "no rule config with compiled patterns found\n");
        leuko_compiled_config_unref(c);
        return 2;
    }

    printf("OK\n");
    leuko_compiled_config_unref(c);
    return 0;
}
