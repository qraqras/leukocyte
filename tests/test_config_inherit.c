#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "configs/discovery/raw_config.h"
#include "configs/discovery/inherit.h"

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
    /* Setup files */
    write_file("tests/tmp_inherit_child.yml", "Layout:\n  IndentationConsistency:\n    Enabled: false\n");
    write_file("tests/tmp_inherit_child2.yml", "Layout:\n  SomeOtherRule:\n    Enabled: true\n");

    /* scalar inherit */
    write_file("tests/tmp_inherit_base.yml", "inherit_from: tests/tmp_inherit_child.yml\n");
    leuko_raw_config_t *base = NULL;
    char *err = NULL;
    int rc = leuko_config_load_file("tests/tmp_inherit_base.yml", &base, &err);
    assert(rc == 0);
    assert(base != NULL);
    leuko_raw_config_t **list = NULL;
    size_t count = 0;
    rc = leuko_config_resolve_inherit_from(base, &list, &count, &err);
    assert(rc == 0);
    assert(count == 1);
    leuko_raw_config_list_free(list, count);
    leuko_raw_config_free(base);

    /* sequence inherit */
    write_file("tests/tmp_inherit_base2.yml", "inherit_from:\n  - tests/tmp_inherit_child.yml\n  - tests/tmp_inherit_child2.yml\n");
    base = NULL;
    err = NULL;
    rc = leuko_config_load_file("tests/tmp_inherit_base2.yml", &base, &err);
    assert(rc == 0);
    rc = leuko_config_resolve_inherit_from(base, &list, &count, &err);
    assert(rc == 0);
    assert(count == 2);
    leuko_raw_config_list_free(list, count);
    leuko_raw_config_free(base);

    /* glob inherit */
    write_file("tests/tmp_inherit_base_glob.yml", "inherit_from: tests/tmp_inherit_child*.yml\n");
    base = NULL;
    err = NULL;
    rc = leuko_config_load_file("tests/tmp_inherit_base_glob.yml", &base, &err);
    assert(rc == 0);
    rc = leuko_config_resolve_inherit_from(base, &list, &count, &err);
    assert(rc == 0);
    assert(count >= 2);
    leuko_raw_config_list_free(list, count);
    leuko_raw_config_free(base);

    /* missing file should error */
    write_file("tests/tmp_inherit_base_missing.yml", "inherit_from: tests/no_such_file.yml\n");
    base = NULL;
    err = NULL;
    rc = leuko_config_load_file("tests/tmp_inherit_base_missing.yml", &base, &err);
    assert(rc == 0);
    rc = leuko_config_resolve_inherit_from(base, &list, &count, &err);
    assert(rc != 0);
    assert(err != NULL);
    free(err);
    leuko_raw_config_free(base);

    /* invalid YAML in child should error */
    write_file("tests/tmp_inherit_badchild.yml", ":\n");
    write_file("tests/tmp_inherit_base_badchild.yml", "inherit_from: tests/tmp_inherit_badchild.yml\n");
    base = NULL;
    err = NULL;
    rc = leuko_config_load_file("tests/tmp_inherit_base_badchild.yml", &base, &err);
    assert(rc == 0);
    rc = leuko_config_resolve_inherit_from(base, &list, &count, &err);
    assert(rc != 0);
    assert(err != NULL);
    free(err);
    leuko_raw_config_free(base);

    return 0;
}
