#ifndef PRISM_XALLOCATOR_H
#define PRISM_XALLOCATOR_H

#include <stddef.h>

/* Function-based allocator API used when PRISM_XALLOCATOR is enabled. */
void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t size);
void *xcalloc(size_t nmemb, size_t size);
void xfree(void *ptr);

/* Per-parse arena lifecycle */
void x_allocator_begin_parse(void);
void x_allocator_end_parse(void);

#endif /* PRISM_XALLOCATOR_H */