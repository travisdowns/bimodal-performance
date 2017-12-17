/*
 * cycle-timer.hpp
 *
 * A timer that returns results in CPU cycles in addition to nanoseconds.
 */

#ifndef CYCLE_TIMER_HPP_
#define CYCLE_TIMER_HPP_

#include <chrono>
#include <string>
#include <cinttypes>
#include <cstdio>
#include <algorithm>

namespace CycleTimer {

namespace impl {

template <typename CLOCK = std::chrono::steady_clock, size_t ITERS = 10000, size_t TRIES = 11, size_t WARMUP = 100>
struct Calibration {
    static volatile size_t sink;

    /**
     * Calibration loop that relies on store throughput being exactly 1 per cycle
     * on all modern x86 chips, and the loop overhead running totally in parallel.
     */
    static void store_calibration(size_t iters) {
        do {
            sink = iters;
        } while (--iters > 0);
    }

    /*
     * Calculate the frequency of the CPU based on timing a tight loop that we expect to
     * take one iteration per cycle.
     *
     * ITERS is the base number of iterations to use: the calibration routine is actually
     * run twice, once with ITERS iterations and once with 2*ITERS, and a delta is used to
     * remove measurement overhead.
     */
    static double getGHzImpl() {
        static_assert(ITERS > 10 && ITERS % 4 == 0, "iters > 10 and multiple of 4 please");

        using ns = std::chrono::nanoseconds::rep;
        std::array<ns, TRIES> results;

        for (size_t w = 0; w < WARMUP + 1; w++) {
            for (size_t r = 0; r < TRIES; r++) {
                auto t0 = CLOCK::now();
                store_calibration(ITERS);
                auto t1 = CLOCK::now();
                store_calibration(ITERS * 2);
                auto t2 = CLOCK::now();
                results[r] = std::chrono::duration_cast<std::chrono::nanoseconds>((t2 - t1) - (t1 - t0)).count();
            }
        }

        // return the median value
        std::sort(results.begin(), results.end());
        double ghz = ((double)ITERS / results[TRIES/2]);
        fprintf(stderr, "Estimated CPU speed: %5.2f GHz\n", ghz);
        return ghz;
    }

    static double getGHz() {
        static double ghz = getGHzImpl();
        return ghz;
    }
};

template <typename CLOCK, size_t ITERS, size_t TRIES, size_t WARMUP>
volatile size_t Calibration<CLOCK, ITERS, TRIES, WARMUP>::sink;

} // namespace impl


/**
 * A point in time, or an interval when subtracted.
 */
class TimingResult {
    int64_t nanos;
public:

    TimingResult(int64_t nanos) : nanos{nanos} {}

    uint64_t getNanos() {
        return nanos;
    }

    uint64_t getCycles() {
        return getNanos() * impl::Calibration<>::getGHz();
    }

    TimingResult operator-(TimingResult rhs) {
        return { nanos - rhs.nanos };
    }
};

/*
 * This class measures cycles indirectly by measuring the wall-time for each test, and then converting
 * that to a cycle count based on a calibration loop performed once at startup.
 */
template <typename CLOCK>
class ClockTimerT {
public:

    typedef int64_t now_t;
    typedef int64_t delta_t;

    /*
     * initialize the clock - this happens automatically when init is necessary (usually lazily - when accessing
     * the TimingResult.getCycles()), but may be lengthy, so this method is offfered so that the user can trigger
     * it at a time of their choosing
     */
    static void init() {
        TimingResult{0}.getCycles();
    }

    /* return a TimeResult object representing the current moment in time */
    static TimingResult now() {
        return {std::chrono::duration_cast<std::chrono::nanoseconds>(CLOCK::now().time_since_epoch()).count()};
    }
};

// the default ClockTimer will use high_resolution_clock
using ClockTimerHiRes = ClockTimerT<std::chrono::high_resolution_clock>;

} // namespace CycleTimer


#endif /* CYCLE_TIMER_HPP_ */
