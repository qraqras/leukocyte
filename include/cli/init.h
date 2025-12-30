/* include/cli/init.h */
#ifndef LEUKO_CLI_INIT_H
#define LEUKO_CLI_INIT_H

#include <stdbool.h>
#include "cli/exit_code.h"

/* Initialize .leukocyte directory under project_dir (if NULL, use cwd).
 * If apply_gitignore is true, attempt to append recommended ignore entries to the
 * project's root .gitignore (explicit action). Returns LEUKO_EXIT_OK on success.
 */
int leuko_cli_init(const char *project_dir, bool apply_gitignore);

#endif /* LEUKO_CLI_INIT_H */
