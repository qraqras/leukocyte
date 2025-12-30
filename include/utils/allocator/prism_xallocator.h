#ifndef PRISM_XALLOCATOR_H
#define PRISM_XALLOCATOR_H

#include <stddef.h>

void *xmalloc(size_t size);
void *xcalloc(size_t nmemb, size_t size);
void *xrealloc(void *ptr, size_t size);
void xfree(void *ptr);

void leuko_x_allocator_begin(void);
void leuko_x_allocator_end(void);

#endif /* PRISM_XALLOCATOR_H */
