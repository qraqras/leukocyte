#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "configs/compiled_config.h"
#include "configs/schema/validator.h"

int main(void)
{
    const char *d = "tests/tmp_schema";
    mkdir(d, 0755);
    char path[1024];
    snprintf(path, sizeof(path), "%s/.rubocop.yml", d);

    /* Invalid: IndentationWidth.Width is a string, LineLength.Max is negative */
    const char *json = "{ \"categories\": { \"Layout\": { \"rules\": { \"IndentationWidth\": { \"Width\": \"foo\" }, \"LineLength\": { \"Max\": -10 } } } } }\n";
    FILE *f = fopen(path, "w");
    assert(f);
    fputs(json, f);
    fclose(f);

    leuko_compiled_config_t *cfg = leuko_compiled_config_build(d, NULL);
    assert(cfg);
    const leuko_node_t *merged = leuko_compiled_config_merged_node(cfg);
    assert(merged);

    leuko_schema_diag_t **arr = NULL;
    size_t count = 0;
    bool valid = leuko_schema_validate_layout(merged, &arr, &count);
    assert(!valid);
    assert(count >= 1);

    for (size_t i = 0; i < count; i++)
    {
        printf("diag %zu: %s - %s\n", i, arr[i]->path, arr[i]->message);
        leuko_schema_diag_free(arr[i]);
    }

    free(arr);

    leuko_compiled_config_unref(cfg);
    remove(path);
    rmdir(d);

    printf("OK\n");
    return 0;
}
