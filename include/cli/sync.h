#ifndef LEUKO_CLI_SYNC_H
#define LEUKO_CLI_SYNC_H

#include "cli/exit_code.h"

/*
 * Run a sync operation that finds the nearest `.rubocop.yml` by searching
 * upward from the current working directory, and invoke the exporter script
 * to write a resolved `.leukocyte.resolved.json` file next to it.
 *
 * Parameters:
 *  - script_path: optional path to exporter script (NULL -> "scripts/export_rubocop_config.rb")
 *  - out_name: optional output filename (NULL -> ".leukocyte.resolved.json")
 *
 * Returns: LEUKO_EXIT_OK on success, LEUKO_EXIT_INVALID on error.
 */
int leuko_sync_run(const char *script_path, const char *out_name);

#endif /* LEUKO_CLI_SYNC_H */
