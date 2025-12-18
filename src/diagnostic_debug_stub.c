/* Diagnostic debug stub used when building embedded Prism
 * Provides weak definitions expected by vendor/prism/src/diagnostic.c
 */
#include <stdarg.h>
#include <stdio.h>
#include <stddef.h>

/* Counter for freed diagnostics (used by Prism diagnostics code) */
size_t g_diag_freed = 0;

/* Simple debug logging macro replacement. Keep signature compatible. */
void LDEBUG(const char *fmt, ...)
{
    /* No-op by default; enable via env var if needed. */
    (void)fmt;
    return;
}
