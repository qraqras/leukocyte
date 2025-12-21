#ifndef LEUKOCYTE_WORKER_PIPELINE_H
#define LEUKOCYTE_WORKER_PIPELINE_H

#include <stdint.h>
#include <stddef.h>
#include "configs/generated_config.h"

/* Run processing pipeline across files using a worker pool.
 * - files: array of file paths
 * - files_count: number of files
 * - cfg: loaded config
 * - workers: number of worker threads to spawn (>=1)
 * - timings: if true, per-file TIMING lines will be printed
 * - any_failures: output count of failed files (optional)
 * - total_parse_ms/total_build_ms/total_visit_ms/total_handler_ns: optional accumulators for totals
 */
bool leuko_run_pipeline(char **files, size_t files_count, config_t *cfg, size_t workers, bool timings, int *any_failures, double *total_parse_ms, double *total_build_ms, double *total_visit_ms, uint64_t *total_handler_ns, int formatter);

#endif /* LEUKOCYTE_WORKER_PIPELINE_H */
