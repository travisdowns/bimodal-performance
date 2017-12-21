/*
 * huge-alloc.hpp
 *
 * Inefficient allocator that allows allocating memory regions backed by THP pages.
 */

#ifndef HUGE_ALLOC_HPP_
#define HUGE_ALLOC_HPP_

#include <cstddef>

// 1152921504606846976 bytes should be enough for everyone
#define MAX_HUGE_ALLOC (1ULL << 60)

/* allocate size bytes of storage in a hugepage */
void *huge_alloc(std::size_t size, bool print);

/* free the pointer pointed to by p */
void huge_free(void *p);

#endif /* HUGE_ALLOC_HPP_ */
