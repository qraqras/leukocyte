#include <stdio.h>
#include <string.h>
#include "../../generated/configs/rules/leuko_layout_indentation_consistency.h"
#include "cJSON.h"

int main(void)
{
    leuko_layout_indentation_consistency_t cfg;
    leuko_layout_indentation_consistency_init_defaults(&cfg);

    /* valid JSON should succeed */
    const char *valid = "{ \"enabled\": false, \"enforced_style\": \"tab\", \"indent_width\": 4 }";
    cJSON *jv = cJSON_Parse(valid);
    if (!jv)
        return 2;
    int r = leuko_layout_indentation_consistency_from_json(&cfg, jv);
    cJSON_Delete(jv);
    if (r != 0)
        return 3;
    if (cfg.enabled != false)
        return 4;
    if (strcmp(cfg.enforced_style, "tab") != 0)
        return 5;
    if (cfg.indent_width != 4)
        return 6;

    /* invalid JSON type should fail */
    const char *bad = "{ \"indent_width\": \"four\" }";
    cJSON *jb = cJSON_Parse(bad);
    if (!jb)
        return 7;
    r = leuko_layout_indentation_consistency_from_json(&cfg, jb);
    cJSON_Delete(jb);
    if (r == 0)
        return 8; /* expected non-zero */

    leuko_layout_indentation_consistency_free(&cfg);
    return 0;
}
