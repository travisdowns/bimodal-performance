/*
 * weirdo-cpp.cpp
 *
 * Put the C++ version of the loop in a separate file to avoid optimizations.
 */
#include <cstdint>
#include <cstdlib>
#include <cinttypes>

#include "pcg-include/pcg_random.hpp"

using namespace std;

volatile int value;

void weirdo_cpp(size_t iters, void* output) {
    using elem_type = uint32_t;
    static_assert(STRIDE % sizeof(elem_type) == 0, "STRIDE in bytes must be a multiple of the element size (4)");
    elem_type x = value;  // read from global so the zero is kept in a register rather than an immediate
    elem_type          *rdx = (elem_type*)output;
    volatile elem_type *rsi = rdx;
    do {
        *rdx = x;
        *rsi = x;

        rdx += (STRIDE / sizeof(elem_type));
    } while (--iters > 0);
}

void rand_writes(size_t iters, void* output) {
    using elem_type = uint32_t;
    using rng_type = pcg32;
    constexpr size_t esize = sizeof(elem_type);

    size_t total_size = iters * STRIDE;
    assert((total_size & (total_size - 1)) == 0); // total_size should be a power of two for the masking to work
    size_t mask = total_size / esize - 1;

    rng_type rng1(1234567u);
    rng_type rng2(5678u);
    elem_type *out = (elem_type *)output;
    elem_type x = value;
    do {
        size_t offset = rng1();
        out[(offset & mask)     ] = x;
        out[(offset & mask) + 64/esize] = x;
        // out[0] = x;
    } while (--iters > 0);
}

// void 

/* always return zero, but hide this here to avoid optimization */
int zero() { return 0; }


