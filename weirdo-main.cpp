/*
 * fixed_decode_standalone.cpp
 */

#include <chrono>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <cinttypes>

#include <sched.h>

#include "cycle-timer.hpp"

bool verbose = true;

typedef void (store_function)(size_t iters, uint32_t* output);
extern "C"
store_function weirdo_asm;
store_function weirdo_cpp;

void *alloc(size_t size) {
//    void *p;
//    assert(posix_memalign(&p, 2 * 1024 * 1024, size) == 0);
//    return p;
    return malloc(size);
}


int main(int argc, char** argv) {
    using cycleclock = CycleTimer::ClockTimerHiRes;

    store_function *function = nullptr;
    if (argc == 2) {
        if      (strcmp(argv[1],"c++") == 0) function = weirdo_cpp;
        else if (strcmp(argv[1],"asm") == 0) function = weirdo_asm;
    }

    if (!function) {
        fprintf(stderr, "Usage:\n\tweirdo-main c++\n\tweirdo-main asm\n");
        exit(EXIT_FAILURE);
    }

    cycleclock::init();

    size_t repeat_count = 20;
    size_t iters = 10000;

    size_t stride       = 16;
    size_t output_size  = 64 * 1024;  // in bytes
    size_t kernel_loops = output_size / stride;

    // set up the dedicated flag, literal and match offset buffers for the helper test
    uint32_t *output = (uint32_t*)alloc(output_size);
    if (verbose) fprintf(stderr, "output size     : %4zu KiB\n", output_size / 1024);
    if (verbose) fprintf(stderr, "output alignment: %4zu\n", (size_t)((1UL << __builtin_ctzl((uintptr_t)output))));

    while (repeat_count-- > 0) {
        int cpubefore = sched_getcpu();
        auto start_cycles = cycleclock::now();
        auto start_cpu    = clock();
        for (int c = iters; c-- > 0;) {
            function(kernel_loops, output);
        }
        printf("%5.2f cycles/iter, %5.2f ns/iter, cpu before: %d, cpu after: %d\n",
                (double)(cycleclock::now() - start_cycles).getCycles() / (kernel_loops * iters),
                (double)(clock() - start_cpu) * 1000000000 / CLOCKS_PER_SEC / (kernel_loops * iters),
                cpubefore,
                sched_getcpu());
    }
    printf("------------\n");
}



