// CLI header
#ifndef LEUKOCYTE_CLI_H
#define LEUKOCYTE_CLI_H

#include <stddef.h>
#include <stdbool.h>

typedef struct
{
    char **paths;
    size_t paths_count;
    char *config_path;
    char *format; // text|json
    char **only;
    size_t only_count;
    char **except;
    size_t except_count;
    bool auto_correct;
} cli_options_t;

// Parse argv into cli_options_t. Returns 0 on success.
int cli_parse(int argc, char *argv[], cli_options_t *opts);
void cli_options_free(cli_options_t *opts);

#endif // LEUKOCYTE_CLI_H
