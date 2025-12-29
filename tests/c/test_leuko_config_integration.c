#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "leuko_config.h"
#include "rules/leuko_layout_indentation_consistency.h"
#include "cJSON.h"

int main(void)
{
    /* Read canonical JSON output from normalize (tests will call normalize before this) */
    FILE *f = fopen("/tmp/leuko_integration.json", "r");
    if (!f)
        return 2;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(len + 1);
    if (!buf)
    {
        fclose(f);
        return 3;
    }
    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);

    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root)
        return 4;

    leuko_config_t cfg;
    leuko_config_init_defaults(&cfg);

    if (leuko_config_from_json(&cfg, root) != 0)
    {
        cJSON_Delete(root);
        return 5;
    }

    /* Expect indentation_consistency overrides from the test input (tab, 4) */
    if (strcmp(cfg.categories.layout.indentation_consistency.enforced_style, "tab") != 0)
    {
        cJSON_Delete(root);
        return 6;
    }
    if (cfg.categories.layout.indentation_consistency.indent_width != 4)
    {
        cJSON_Delete(root);
        return 7;
    }

    leuko_config_free(&cfg);
    cJSON_Delete(root);
    return 0;
}
