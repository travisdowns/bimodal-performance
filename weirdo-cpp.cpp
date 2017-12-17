/*
 * weirdo-cpp.cpp
 *
 * Put the C++ version of the loop in a separate file to avoid optimizations.
 */
#include <cstdint>
#include <cstdlib>

volatile uint32_t value;

void weirdo_cpp(size_t iters, uint32_t* output) {

    uint32_t x = value;  // read from global so the zero is kept in a register rather than an immediate
    uint32_t          *rdx = output;
    volatile uint32_t *rsi = output;
    do {
        *rdx = x;
        *rsi = x;

        rdx += 4;  // 16 byte stride
    } while (--iters > 0);
}




