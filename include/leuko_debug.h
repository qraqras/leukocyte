/*
 * leuko_debug.h
 * Minimal declarations expected by vendor Prism sources.
 */

#ifndef LEUKO_DEBUG_H
#define LEUKO_DEBUG_H

#include <stddef.h>

/* Diagnostic counters (defined in diagnostic_debug_stub.c or equivalent) */
extern size_t g_diag_created;
extern size_t g_diag_freed;

/* Debug logging hook provided by the project. */
void leuko_debug_log(const char *fmt, ...);

#ifndef LDEBUG
#define LDEBUG(...) leuko_debug_log(__VA_ARGS__)
#endif

#endif /* LEUKO_DEBUG_H */
