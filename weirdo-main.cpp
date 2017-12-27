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
#include <sys/mman.h>

#include "cycle-timer.hpp"
#include "huge-alloc.hpp"

bool getBool(const char *var, bool def = false) {
    const char *val = getenv(var);
    return val ? strcmp(val,"1") == 0 : def;
}

bool verbose = getBool("W_VERBOSE");
bool summary = getBool("W_SUMMARY");
bool domax   = getBool("W_MAX");
bool color   = getBool("W_COLOR", true);
int cpu = 1;

typedef void (store_function)(size_t iters, void* output);
extern "C" {
store_function weirdo_write;
store_function weirdo_write2;  // two streams both in L2
store_function weirdo_write3;  // L2 stream + moving L1 (stride 1) stream
store_function weirdo_write_pf;
store_function weirdo_read1;
store_function weirdo_read2;
}
store_function weirdo_cpp;

void *alloc(size_t size) {
    size_t grossed_up = size * 2 + 1000;

//    void *p;
//    assert(posix_memalign(&p, 2 * 1024 * 1024, grossed_up) == 0);
//    madvise(p, 2 * 1024 * 1024, MADV_HUGEPAGE);
//    return p;
//    return malloc(size);
    return huge_alloc(grossed_up, !summary);
}

void pinToCpu(int cpu) {
    cpu_set_t set;
    CPU_ZERO(&set);
//    for (int i = 0; i < 100; i++) {
//        CPU_SET(i, &set);
//    }
    CPU_SET(cpu, &set);
    if (sched_setaffinity(0, sizeof(set), &set)) {
        assert("pinning failed" && false);
    }
}

int zero();

void usageError() {
    fprintf(stderr,
            "Usage:\n"
            "\tweirdo-main c++    [summary]\n"
            "\tweirdo-main asm    [summary]\n"
            "\tweirdo-main write2 [summary]\n"
            "\tweirdo-main read1  [summary]\n"
            "\tweirdo-main read2  [summary]\n"
            );
    exit(EXIT_FAILURE);
}

const char *   RED = "\033[0;31m";
const char * GREEN = "\033[0;32m";
const char *YELLOW = "\033[0;33m";
const char *MAGENT = "\033[0;95m";

const char *NC="\033[0m";

//  // suitable for 2 stores
//const char* selectColor(double v) {
//    if (v <= 2.4) return NC;
//    if (v <= 3.35) return YELLOW;
//    if (v <= 3.6) return RED;
//    return MAGENT;
//}

// suitable for 3 stores
const char* selectColor(double v) {
    if (color) {
        // suitable for 2 stores - stride 64
        if (v <= 4.2) return NC;
        if (v <= 7) return GREEN;
        if (v <= 8) return YELLOW;
        if (v <= 10) return RED;
        return MAGENT;
//        if (v <= 4.5) return NC;
//        if (v <= 5.5) return GREEN;
//        if (v <= 6.3) return YELLOW;
//        if (v <= 10)  return RED;
//        return MAGENT;
    }
    return "";
}


int main(int argc, char** argv) {
    using cycleclock = CycleTimer::ClockTimerHiRes;

    if (argc == 3) {
        if (strcmp(argv[2],"summary")) {
            fprintf(stderr, "Bad second arg: '%s'", argv[2]);
            usageError();
        }
        argc--;
        verbose = false;
        summary = true;
    }
    store_function *function = nullptr;
    if (argc == 2) {
        if      (strcmp(argv[1],"c++")  == 0)   function = weirdo_cpp;
        else if (strcmp(argv[1],"asm")  == 0)   function = weirdo_write;
        else if (strcmp(argv[1],"write2") == 0) function = weirdo_write2;
        else if (strcmp(argv[1],"write3") == 0) function = weirdo_write3;
        else if (strcmp(argv[1],"asm_pf") == 0) function = weirdo_write_pf;
        else if (strcmp(argv[1],"read1") == 0)  function = weirdo_read1;
        else if (strcmp(argv[1],"read2") == 0)  function = weirdo_read2;
    }

    if (!function) {
        usageError();
    }

    pinToCpu(cpu);
//    fprintf(stderr, "pinned to cpu %d\n", cpu);

    cycleclock::init(!summary);

    size_t repeat_count = 10;
    size_t iters = 5000;

    size_t stride       = STRIDE;
    size_t output_size  = 64 * 1024;  // in bytes
    size_t kernel_loops = output_size / stride;

    // set up the dedicated flag, literal and match offset buffers for the helper test
    void *output = alloc(output_size * 2);
    memset(output, zero(), output_size * 2 + 1000);
    if (verbose) fprintf(stderr, "output size     : %4zu KiB\n", output_size / 1024);
    if (verbose) fprintf(stderr, "output alignment: %4zu\n", (size_t)((1UL << __builtin_ctzl((uintptr_t)output))));

    double min_cycles = domax ? 0 : UINT64_MAX;

    while (repeat_count-- > 0) {
        int cpubefore = sched_getcpu();
        auto start_cycles = cycleclock::now();
        auto start_cpu    = clock();
        for (int c = iters; c-- > 0;) {
            function(kernel_loops, output);
        }
        auto end_cycles = cycleclock::now();
        auto end_cpu    = clock();
        double cycles = (end_cycles - start_cycles).getCycles();
        if (!summary) printf("%5.2f cycles/iter, %5.2f ns/iter, %5.2f cycles/line, cpu switch: %s\n",
                cycles / (kernel_loops * iters),
                (double)(end_cpu - start_cpu) * 1000000000 / CLOCKS_PER_SEC / (kernel_loops * iters),
                cycles / (kernel_loops * iters * STRIDE / 64),
                cpubefore == sched_getcpu() ? "---" : "YES"
                );
        min_cycles  = domax ? std::max(min_cycles, cycles) : std::min(min_cycles, cycles);
    }
    if (!summary) printf("------------\n");

    if (summary) {
        double cycles = min_cycles / (kernel_loops * iters);
        const char* color = selectColor(cycles);
        printf("%s%6.1f%s", color, cycles, NC);
    }
}



