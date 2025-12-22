#ifndef LEUKOCYTE_WORKER_PIPELINE_H
#define LEUKOCYTE_WORKER_PIPELINE_H

#include <stdint.h>
#include <stddef.h>
#include "configs/config.h"

/* Run processing pipeline across files using a worker pool.
 * - files: array of file paths
 * - files_count: number of files
 * - cfg: loaded config
 * - workers: number of worker threads to spawn (>=1)
 * - any_failures: output count of failed files (optional)
 * - formatter: formatter to use for printing diagnostics
 */
bool leuko_run_pipeline(char **files, size_t files_count, leuko_config_t *cfg, size_t workers, int *any_failures, int formatter);

#endif /* LEUKOCYTE_WORKER_PIPELINE_H */
