#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include "leuko_debug.h"

/* Diagnostic counters (visible to other translation units) */
size_t g_diag_created = 0;
size_t g_diag_freed = 0;

/* Simple debug logging implementation. Prints to stderr. */
void leuko_debug_log(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}
