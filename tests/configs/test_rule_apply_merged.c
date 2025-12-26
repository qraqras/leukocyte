#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "configs/compiled_config.h"
#include "common/generated_rules.h"
#include "configs/common/rule_config.h"
#include "configs/layout/indentation_width.h"
#include "configs/layout/indentation_style.h"
#include "configs/layout/line_length.h"

/* Forward-declare helper to access rule config by global index (not in header) */
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
    const char *d = "tests/tmp_rule_apply";
    mkdir(d, 0755);
    char path[1024];
    snprintf(path, sizeof(path), "%s/.rubocop.yml", d);
    const char *json = "{ \"Layout\": { \"IndentationWidth\": { \"Width\": 4 }, \"IndentationStyle\": { \"EnforcedStyle\": \"tab\" }, \"LineLength\": { \"Max\": 100 } } }\n";
    write_file(path, json);

    leuko_compiled_config_t *cfg = leuko_compiled_config_build(d, NULL);
    assert(cfg);
    const leuko_config_t *ef = leuko_compiled_config_rules(cfg);
    assert(ef);

    size_t idx_w = leuko_rule_find_index("Layout", "IndentationWidth");
    assert(idx_w != SIZE_MAX);
    leuko_rule_config_t *rconf_w = leuko_rule_config_get_by_index((leuko_config_t *)ef, idx_w);
    assert(rconf_w);
    layout_indentation_width_config_t *sc_w = (layout_indentation_width_config_t *)rconf_w->specific_config;
    assert(sc_w);
    assert(sc_w->width == 4);

    size_t idx_s = leuko_rule_find_index("Layout", "IndentationStyle");
    assert(idx_s != SIZE_MAX);
    leuko_rule_config_t *rconf_s = leuko_rule_config_get_by_index((leuko_config_t *)ef, idx_s);
    assert(rconf_s);
    layout_indentation_style_config_t *sc_s = (layout_indentation_style_config_t *)rconf_s->specific_config;
    assert(sc_s);
    assert(sc_s->style == LAYOUT_INDENTATION_STYLE_TABS);

    size_t idx_l = leuko_rule_find_index("Layout", "LineLength");
    assert(idx_l != SIZE_MAX);
    leuko_rule_config_t *rconf_l = leuko_rule_config_get_by_index((leuko_config_t *)ef, idx_l);
    assert(rconf_l);
    layout_line_length_config_t *sc_l = (layout_line_length_config_t *)rconf_l->specific_config;
    assert(sc_l);
    assert(sc_l->max == 100);

    leuko_compiled_config_unref(cfg);
    remove(path);
    rmdir(d);

    printf("OK\n");
    return 0;
}
