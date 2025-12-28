#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "configs/common/rule_config.h"
#include "configs/common/config.h"
#include "configs/layout/indentation_width.h"

int main(void)
{
    /* Create source heap-initialized rule */
    void *src = layout_indentation_width_initialize();
    assert(src);
    layout_indentation_width_config_t *orig = &((leuko_config_rule_view_indentation_width_t *)src)->specific;
    assert(orig);
    orig->width = 4;

    /* snapshot bytes of source specific */
    uint8_t snapshot[sizeof(*orig)];
    memcpy(snapshot, orig, sizeof(snapshot));

    /* create a target config and move into it */
    leuko_config_t *cfg = calloc(1, sizeof(*cfg));
    assert(cfg);
    leuko_config_initialize(cfg);

    leuko_config_set_view_rule(cfg, "Layout", "IndentationWidth", src);

    /* read back embedded view */
    void *rview = leuko_config_get_view_rule(cfg, "Layout", "IndentationWidth");
    assert(rview);
    layout_indentation_width_config_t *dst = &((leuko_config_rule_view_indentation_width_t *)rview)->specific;
    assert(dst);
    assert(dst->width == 4);

    /* ensure full-bytes match snapshot */
    assert(memcmp(snapshot, dst, sizeof(snapshot)) == 0);

    leuko_config_free(cfg);
    printf("OK\n");
    return 0;
}
