#ifndef LEUKO_CLI_INIT_H
#define LEUKO_CLI_INIT_H

#include <stdbool.h>
#include "cli/exit_code.h"

int leuko_cli_init(const char *project_dir, bool apply_gitignore);

#endif /* LEUKO_CLI_INIT_H */
