#ifndef PRISM_XALLOCATOR_H
#define PRISM_XALLOCATOR_H

#include <stddef.h>

/* Prism allocator hooks (use x* names) */
void *xmalloc(size_t size);
void *xcalloc(size_t nmemb, size_t size);
void *xrealloc(void *ptr, size_t size);
void xfree(void *ptr);

#endif /* PRISM_XALLOCATOR_H */
