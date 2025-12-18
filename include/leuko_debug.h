#ifndef LEUKO_DEBUG_H
#define LEUKO_DEBUG_H

#include <stddef.h>

/* Simple debug logging used by embedded Prism diagnostics */
void LDEBUG(const char *fmt, ...);
extern size_t g_diag_freed;

#endif /* LEUKO_DEBUG_H */
