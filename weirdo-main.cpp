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

#include <emmintrin.h>

int getenv_int(const char *var, int def) {
    const char *val = getenv(var);
    return val ? std::atoi(val) : def;
}

bool getenv_bool(const char *var, bool def = false) {
    const char *val = getenv(var);
    return val ? strcmp(val, "1") == 0 : def;
}

bool verbose    = getenv_bool("W_VERBOSE");
bool summary    = getenv_bool("W_SUMMARY");
bool domax      = getenv_bool("W_MAX");
bool color      = getenv_bool("W_COLOR", true);
bool dump_tests_flag = getenv_bool("W_DUMPTESTS");

int region_kib = getenv_int("W_REGIONKIB", 64);
int pincpu     = getenv_int("W_PINCPU",    1);

typedef void (store_function)(size_t iters, void* output);

struct func_descriptor {
    const char *name;
    store_function *f;
    const char *desc;
    // expected number of L1 and L2 hits (L2 hits implying L1 misses)
    int l1_hits;
    int l2_hits;
    // a multiplier applied to the number of "kernel loops" (accesses the test is
    // request to perform), useful for tests that have sigifnicant startup overhead
    int loop_mul;
};


extern "C" {
store_function weirdo_write;
store_function weirdo_write2;
store_function weirdo_write3;
store_function weirdo_write4;
store_function weirdo_write_pf;
store_function weirdo_read1;
store_function weirdo_read2;
store_function linear;
store_function rand_asm;
store_function rand_asm2;
store_function write_aabb;
store_function write_abab;
}

store_function weirdo_cpp;
store_function rand_writes;
store_function populate_set;

//                                                                                       /---------- l1_hits
//                                                                                       |  /------- l2_hits
//                                                                                       |  |   /--- loop_mul
const func_descriptor all_funcs[] = {   //                                               v  v   v
        { "c++"       , weirdo_cpp     , "c++ version of fixed L1 + 16-stride L2 writes", 1, 1,  1 },
        { "asm"       , weirdo_write   , "64-bit stride interleaved 2xL2 accesses"      , 0, 2,  1 },
        { "write2"    , weirdo_write2  , "two streams both in L2"                       , 0, 2,  1 },
        { "write3"    , weirdo_write3  , "Single L2 stream"                             , 0, 1,  1 },
        { "write4"    , weirdo_write4  , "L2 stream + fixed L1"                         , 1, 1,  1 },
        { "asm_pf"    , weirdo_write_pf, "like asm, but with prefetching"               , 0, 2,  1 },
        { "read1"     , weirdo_read1   , "read1"                                        , 0, 0,  1 },
        { "read2"     , weirdo_read2   , "read2"                                        , 0, 0,  1 },
        { "linear"    , linear         , "single stream of strided reads"               , 0, 1,  1 },
        { "rand"      , rand_writes    , "C++ random writes"                            , 1, 1,  1 },
        { "rand-asm"  , rand_asm       , "asm random writes"                            , 1, 1,  1 },
        { "rand-asm2" , rand_asm2      , "asm random writes2"                           , 1, 1,  1 },
        { "write_aabb", write_aabb     , "32-byte stride L2 writes AABB pattern"        , 1, 1,  1 },
        { "write_abab", write_abab     , "32-byte stride L2 writes ABAB pattern"        , 1, 1,  1 },
        { "pop-set"   , populate_set   , "pop-set: writes to 2 sets"                    , 1, 1, 20 },
        { nullptr, nullptr, nullptr, 0, 0, 0 }  // sentinel
};

void *alloc(size_t size) {
    size_t grossed_up = size * 2 + 1000;

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
            "\tweirdo-main TEST_NAME\n"
            "\n TEST_NAME is one of:\n\n"
            );

    for (const func_descriptor* desc = all_funcs; desc->name; desc++) {
        printf(" %s\n\t%s\n", desc->name, desc->desc);
    }
    exit(EXIT_FAILURE);
}

const char *RED    = "\033[0;31m";
const char *GREEN  = "\033[0;32m";
const char *YELLOW = "\033[0;33m";
const char *MAGENT = "\033[0;95m";
const char *NC     = "\033[0m";


const char* selectColor(double v) {
    if (color) {
        // suitable for 2 stores - stride 64
        if (v <= 4.2) return NC;
        if (v <= 7) return GREEN;
        if (v <= 8) return YELLOW;
        if (v <= 10) return RED;
        return MAGENT;
        /*
        // suitable for 3 stores
        if (v <= 4.5) return NC;
        if (v <= 5.5) return GREEN;
        if (v <= 6.3) return YELLOW;
        if (v <= 10)  return RED;
        return MAGENT;
        */
    }
    return "";
}

/* dump the tests in a single space-separate strings, perhaps convenient so you can do something like:
       for test in $(W_DUMPTESTS=1 ./weirdo-main); do ./weirdo-main $test; done
    to run all tests. */
void dump_tests() {
    for (const func_descriptor* desc = all_funcs; desc->name; desc++) {
        printf("%s ", desc->name);
    }
}

int main(int argc, char** argv) {
    using cycleclock = CycleTimer::ClockTimerHiRes;

    if (argc >= 3) {
        fprintf(stderr, "Unexpected second argument: '%s'\n", argv[2]);
        usageError();
    }

    if (dump_tests_flag) {
        dump_tests();
        exit(EXIT_SUCCESS);
    }

    const char* fname = argc >= 2 ? argv[1] : "asm";
    const func_descriptor *test = nullptr;
    for (const func_descriptor* desc = all_funcs; desc->name; desc++) {
        if (strcmp(desc->name, fname) == 0) {
            test = desc;
            break;
        }
    }

    if (!test) {
        fprintf(stderr, "Bad test name: %s\n", fname);
        usageError();
    }

    if (argc > 3) {
        fprintf(stderr, "Extraneous arguments ignored");
    }

    pinToCpu(pincpu);

    cycleclock::init(!summary);

    // run the whole test repeat_count times, each of which calls the test function iters times,
    // and each test function should loop kernel_loops times (with 1 or 2 stores), or equivalent with unrolling
    size_t repeat_count = 10;
    size_t iters = 500;
    size_t output_size  = region_kib * 1024;  // in bytes
    size_t kernel_loops = output_size / (STRIDE ? STRIDE : 1) * test->loop_mul;

    // set up the dedicated flag, literal and match offset buffers for the helper test
    void *output = alloc(output_size * 2);
    memset(output, zero(), output_size * 2 + 1000);

    if (!summary) {
        fprintf(stderr, "Running test %s : %s\n", test->name, test->desc);
    }

    if (verbose)  {
        fprintf(stderr, "pinned cpu  : %3d\n", pincpu);
        fprintf(stderr, "stride      : %3d\n", STRIDE);
        fprintf(stderr, "output size : %3zu KiB\n", output_size / 1024);
        fprintf(stderr, "output align: %3zu\n", (size_t)((1UL << __builtin_ctzl((uintptr_t)output))));
        fprintf(stderr, "expected number of L1 store hits: %zu\n", (size_t)iters * kernel_loops * repeat_count * test->l1_hits);
        fprintf(stderr, "expected number of L2 store hits: %zu\n", (size_t)iters * kernel_loops * repeat_count * test->l2_hits);
    }

    if (!summary) {
        fprintf(stderr, "Starting main loop after %zu ms\n", (size_t)clock() * 1000u / CLOCKS_PER_SEC);
    }

    double min_cycles = domax ? 0 : UINT64_MAX;

    while (repeat_count-- > 0) {
        int cpubefore = sched_getcpu();
        auto start_cycles = cycleclock::now();
        auto start_cpu    = clock();
        for (int c = iters; c-- > 0;) {
            test->f(kernel_loops, output);
            _mm_lfence(); // prevent inter-iteration overlap
        }
        auto end_cycles = cycleclock::now();
        auto end_cpu    = clock();
        double cycles = (end_cycles - start_cycles).getCycles();
        if (!summary) printf("%5.2f cycles/line, %5.2f cycles/iter, %5.2f cpu ns/iter, cpu switch: %s\n",
                cycles / ((double)kernel_loops * iters * STRIDE / 64),
                cycles / (kernel_loops * iters),
                (double)(end_cpu - start_cpu) * 1000000000 / CLOCKS_PER_SEC / (kernel_loops * iters),
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



