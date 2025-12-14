#include <assert.h>
#include <stdio.h>

#include "leukocyte.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "configs/config_defaults.h"
#include "configs/config_file.h"
#include "configs/severity.h"

static void test_yaml_apply()
{
    const char *yaml = "Layout/IndentationConsistency:\n"
                       "  Enabled: false\n"
                       "  Severity: warning\n"
                       "  EnforcedStyle: indented_internal_methods\n"
                       "  Include:\n"
                       "    - lib/**\n"
                       "  Exclude:\n"
                       "    - test/**\n";

    char tmpfile[] = "/tmp/leuko_test_yaml_XXXXXX";
    int fd = mkstemp(tmpfile);
    if (fd < 0)
        abort();
    FILE *f = fdopen(fd, "w");
    if (!f)
        abort();
    fprintf(f, "%s", yaml);
    fclose(f);

    file_t *cfg = file_clone(&config_defaults);
    if (!cfg)
        abort();

    char *err = NULL;
    bool ok = config_file_apply_yaml(cfg, tmpfile, &err);
    if (!ok)
    {
        fprintf(stderr, "YAML apply failed: %s\n", err ? err : "(no msg)");
        free(err);
        unlink(tmpfile);
        file_free(cfg);
        abort();
    }

    // Check results
    if (cfg->IndentationConsistency_common.enabled != false)
        abort();
    if (cfg->IndentationConsistency_common.severity != SEVERITY_WARNING)
        abort();
    if (!cfg->IndentationConsistency)
        abort();
    if (cfg->IndentationConsistency->enforced_style != INDENTATION_CONSISTENCY_ENFORCED_STYLE_INDENTED_INTERNAL_METHODS)
        abort();
    if (cfg->IndentationConsistency_common.include_count != 1)
        abort();
    if (strcmp(cfg->IndentationConsistency_common.include[0], "lib/**") != 0)
        abort();
    if (cfg->IndentationConsistency_common.exclude_count != 1)
        abort();
    if (strcmp(cfg->IndentationConsistency_common.exclude[0], "test/**") != 0)
        abort();

    // cleanup
    unlink(tmpfile);
    file_free(cfg);
}

int main()
{
    printf("Running tests for Leukocyte...\n");

    test_yaml_apply();

    // Placeholder for additional tests

    printf("All tests passed!\n");
    return 0;
}
