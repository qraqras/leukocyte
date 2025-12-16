#ifndef LEUKOCYTE_CONFIGS_DIAGNOSTICS_H
#define LEUKOCYTE_CONFIGS_DIAGNOSTICS_H

#include <prism/util/pm_list.h>
#include <stdio.h>

/** Append a diagnostics message to the given list. The list owns the message.
 * Caller may pass a NULL list to skip diagnostics collection.
 */
void config_diagnostics_append(pm_list_t *list, int line, int column, const char *fmt, ...);

/** Return the first diagnostics message in the list, or NULL if empty. The
 * returned pointer is owned by the list and must not be freed by the caller.
 */
const char *config_diagnostics_first_message(pm_list_t *list);

/** Print all diagnostics messages (if any) to the given FILE stream. */
void config_diagnostics_print_all(FILE *out, pm_list_t *list);
/** Free diagnostics messages and underlying list nodes. */
void config_diagnostics_free(pm_list_t *list);

#endif /* LEUKOCYTE_CONFIGS_DIAGNOSTICS_H */
