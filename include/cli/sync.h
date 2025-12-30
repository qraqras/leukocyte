/* include/cli/sync_cmd.h */
#ifndef LEUKO_CLI_SYNC_CMD_H
#define LEUKO_CLI_SYNC_CMD_H

#include "cli/exit_code.h"

/*
 * Run the sync operation which invokes the Ruby script to find RuboCop configs
 * and generate JSON outputs under .leukocyte. Parameters may be NULL to use
 * sensible defaults (cwd, default script path, default outdir/index).
 * Returns LEUKO_EXIT_OK on success, LEUKO_EXIT_INVALID on failure.
 */
int leuko_cli_sync(const char *project_dir, const char *script_path, const char *outdir, const char *index_path);

#endif /* LEUKO_CLI_SYNC_CMD_H */
