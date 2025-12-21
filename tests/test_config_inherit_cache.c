#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "configs/raw_config.h"
#include "configs/inherit.h"

int main(void)
{
    /* Create parent and child configs */
    FILE *f = fopen("tests/tmp_cache_parent.yml", "wb");
    assert(f);
    fprintf(f, "Layout:\n  SomeRule:\n    Enabled: true\n");
    fclose(f);

    f = fopen("tests/tmp_cache_child.yml", "wb");
    assert(f);
    fprintf(f, "inherit_from: tmp_cache_parent.yml\nLayout:\n  SomeRule:\n    Enabled: false\n");
    fclose(f);

    leuko_raw_config_t *base = NULL;
    char *err = NULL;
    assert(leuko_config_load_file("tests/tmp_cache_child.yml", &base, &err) == 0);

    leuko_raw_config_t **parents = NULL; size_t parent_count = 0;
    assert(leuko_config_collect_inherit_chain(base, &parents, &parent_count, &err) == 0);
    assert(parent_count == 1);
    leuko_raw_config_list_free(parents, parent_count);

    /* Call again to exercise cache hit path (no modifications) */
    assert(leuko_config_collect_inherit_chain(base, &parents, &parent_count, &err) == 0);
    assert(parent_count == 1);
    leuko_raw_config_list_free(parents, parent_count);

    /* Modify parent file (touch / write) to change mtime and ensure cache invalidation */
    sleep(1);
    f = fopen("tests/tmp_cache_parent.yml", "wb");
    assert(f);
    fprintf(f, "Layout:\n  SomeRule:\n    Enabled: true\n    Note: modified\n");
    fclose(f);

    int rc = leuko_config_collect_inherit_chain(base, &parents, &parent_count, &err);
    assert(rc == 0);
    assert(parent_count == 1);
    leuko_raw_config_list_free(parents, parent_count);

    leuko_raw_config_free(base);
    return 0;
}
