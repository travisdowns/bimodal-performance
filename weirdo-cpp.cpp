/*
 * weirdo-cpp.cpp
 *
 * Put the C++ version of the loop in a separate file to avoid optimizations.
 */
#include <cstdint>
#include <cstdlib>
#include <cinttypes>
#include <cstring>

#include "opt-control.h"
#include "huge-alloc.hpp"
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

template <typename T>
constexpr bool is_pow2(T t) {
    return t > 0 && (t & (t-1)) == 0;
}
template <int span, int max>
class approximate_set {
    static_assert(max >= span, "max >= span required");
    static_assert(is_pow2(span) && is_pow2(max), "max and span should be a power of two");

    static constexpr int ARRAY_SIZE = max / span;
    unsigned char *used;

    size_t to_idx(int value) {
        return (value & (ARRAY_SIZE-1)) / span;
    }

    approximate_set(const approximate_set&) = delete;
    void operator=(const approximate_set&) = delete;

    static std::vector<unsigned char *> alloc_cache;

    unsigned char *allocate() {
        if (alloc_cache.empty()) {
            return static_cast<unsigned char*>(huge_alloc(ARRAY_SIZE, false));
        } else {
            auto ret = alloc_cache.back();
            alloc_cache.pop_back();
            return ret;
        }
    }

public:
    approximate_set() {
        used = allocate();
        // you'd hope that zero-init for arrays would compile to the same thing, but it doesn't
        // it uses ARRAY_SIZE single-byte stores in a loop :(
        std::memset(used, 0, ARRAY_SIZE);
        // printf("set size %d\n", ARRAY_SIZE);
    }

    ~approximate_set() {
        alloc_cache.push_back(used);
    }


    void accept(int value) {
        used[to_idx(value)] = 1;
    }

    /* the total number of used spans */
    int count() {
        return count_span(0, max);
    }

    int count_span(int from, int to) {
        return static_cast<int>(ARRAY_SIZE - std::count(used + to_idx(from), used + to_idx(to - 1) + 1, 0));
    }

    unsigned char *array() {
        return used;
    }
};

template <int span, int max>
std::vector<unsigned char*> approximate_set<span, max>::alloc_cache;

void populate_set(size_t iters, void *) {
    approximate_set<1, 1 << 20> s1;
    approximate_set<1, 1 << 12> s2;
    pcg32_fast rng;
    do {
        auto val = rng();
        s1.accept(val);
        s2.accept(val);
    } while (--iters != 0);
    sink_ptr(s1.array());
}

int countol(approximate_set<1,2> &s) {
    return s.count();
}

/* always return zero, but hide this here to avoid optimization */
int zero() { return 0; }


