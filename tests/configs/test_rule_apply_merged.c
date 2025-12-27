#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "configs/compiled_config.h"
#include "common/rule_registry.h"
#include "configs/common/rule_config.h"
#include "configs/layout/indentation_width.h"
#include "configs/layout/indentation_style.h"
#include "configs/layout/line_length.h"

/* Forward-declare minimal types used in this test */
typedef struct leuko_config_s leuko_config_t;
/* Use base rule type for view access */
typedef struct leuko_config_rule_base_s leuko_config_rule_base_t;

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
    const char *json = "{ \"categories\": { \"Layout\": { \"rules\": { \"IndentationWidth\": { \"Width\": 4 }, \"IndentationStyle\": { \"EnforcedStyle\": \"tab\" }, \"LineLength\": { \"Max\": 100 } } } } }\n";
    write_file(path, json);

    leuko_compiled_config_t *cfg = leuko_compiled_config_build(d, NULL);
    assert(cfg);
    const leuko_config_t *ef = leuko_compiled_config_rules(cfg);
    assert(ef);

    leuko_config_rule_view_t *rconf_w = leuko_compiled_config_view_rule(cfg, "Layout", "IndentationWidth");
    assert(rconf_w);
    layout_indentation_width_config_t *sc_w = &((leuko_config_rule_view_indentation_width_t *)rconf_w)->specific;
    assert(sc_w->width == 4);

    leuko_config_rule_view_t *rconf_s = leuko_compiled_config_view_rule(cfg, "Layout", "IndentationStyle");
    assert(rconf_s);
    layout_indentation_style_config_t *sc_s = &((leuko_config_rule_view_indentation_style_t *)rconf_s)->specific;
    assert(sc_s->style == LAYOUT_INDENTATION_STYLE_TABS);

    leuko_config_rule_view_t *rconf_l = leuko_compiled_config_view_rule(cfg, "Layout", "LineLength");
    assert(rconf_l);
    layout_line_length_config_t *sc_l = &((leuko_config_rule_view_line_length_t *)rconf_l)->specific;
    assert(sc_l->max == 100);
    leuko_compiled_config_unref(cfg);
    remove(path);
    rmdir(d);

    printf("OK\n");
    return 0;
}
