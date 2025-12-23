#ifndef LEUKO_COMMON_EXIT_CODE_H
#define LEUKO_COMMON_EXIT_CODE_H

/* RuboCop-compatible exit codes
 * 0: no offenses found or below fail-level
 * 1: offenses found (>= fail-level)
 * 2: abnormal termination (invalid config, invalid CLI options, internal error)
 */
#define LEUKO_EXIT_OK 0
#define LEUKO_EXIT_OFFENSES 1
#define LEUKO_EXIT_INVALID 2

#endif /* LEUKO_COMMON_EXIT_CODE_H */
