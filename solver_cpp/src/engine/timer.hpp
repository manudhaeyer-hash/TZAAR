#ifndef TZAAR_ENGINE_TIMER_HPP
#define TZAAR_ENGINE_TIMER_HPP

#include <chrono>
#include "../core/types.hpp"

namespace tzaar {

class Timer {
public:
    void start() { t0_ = clock::now(); }
    i64 us() const {
        return std::chrono::duration_cast<std::chrono::microseconds>(clock::now() - t0_).count();
    }
    double ms() const { return us() / 1000.0; }
private:
    using clock = std::chrono::steady_clock;
    clock::time_point t0_ = clock::now();
};

} // namespace tzaar
#endif
