#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "configs/raw_config.h"

static int write_file(const char *path, const char *content)
{
    FILE *f = fopen(path, "wb");
    if (!f)
        return 0;
    fwrite(content, 1, strlen(content), f);
    fclose(f);
    return 1;
}

int main(void)
{
    leuko_raw_config_t *cfg = NULL;
    char *err = NULL;

    /* valid file */
    const char *tmp = "tests/tmp_valid.yml";
    write_file(tmp, "Layout:\n  IndentationConsistency:\n    Enabled: false\n");
    int rc = leuko_config_load_file(tmp, &cfg, &err);
    assert(rc == 0);
    assert(cfg != NULL);
    assert(err == NULL);
    leuko_raw_config_free(cfg);

    /* invalid YAML */
    const char *tmp2 = "tests/tmp_invalid.yml";
    write_file(tmp2, ":\n");
    cfg = NULL; err = NULL;
    rc = leuko_config_load_file(tmp2, &cfg, &err);
    assert(rc != 0);
    assert(cfg == NULL);
    assert(err != NULL);
    assert(strcmp(err, "failed to parse YAML config") == 0);
    free(err);

    /* non-existent file */
    cfg = NULL; err = NULL;
    rc = leuko_config_load_file("tests/no_such_file.yml", &cfg, &err);
    assert(rc != 0);
    assert(cfg == NULL);
    assert(err != NULL);
    assert(strcmp(err, "could not read config file") == 0);
    free(err);

    return 0;
}
