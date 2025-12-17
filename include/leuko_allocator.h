#ifndef LEUKO_ALLOCATOR_H
#define LEUKO_ALLOCATOR_H

#include <stddef.h>

/* Called before parsing a file to prepare per-parse arena state. */
void leuko_allocator_begin_parse(void);

/* Called after parsing a file to release per-parse arena state. */
void leuko_allocator_end_parse(void);

/* PRISM allocator hooks */
void *leuko_xmalloc(size_t size);
void *leuko_xcalloc(size_t nmemb, size_t size);
void *leuko_xrealloc(void *ptr, size_t size);
void leuko_xfree(void *ptr);

#endif /* LEUKO_ALLOCATOR_H */