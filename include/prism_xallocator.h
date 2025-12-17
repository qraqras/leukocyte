#ifndef PRISM_XALLOCATOR_H
#define PRISM_XALLOCATOR_H

#include <stddef.h>

/* Leuko-provided allocator hooks used when building Prism with -DPRISM_XALLOCATOR. */
void *leuko_xmalloc(size_t size);
void *leuko_xcalloc(size_t nmemb, size_t size);
void *leuko_xrealloc(void *ptr, size_t size);
void leuko_xfree(void *ptr);

/* Map prism's x* names to Leuko's implementations */
#define xmalloc leuko_xmalloc
#define xcalloc leuko_xcalloc
#define xrealloc leuko_xrealloc
#define xfree leuko_xfree

#endif /* PRISM_XALLOCATOR_H */