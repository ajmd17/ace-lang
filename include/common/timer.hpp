#ifndef TIMER_HPP
#define TIMER_HPP

#include <chrono>
#include <ratio>
#include <cstdint>

namespace ace {
class Timer {
public:
    std::int64_t Start() {
        m_start = std::chrono::high_resolution_clock::now();
        return m_start.time_since_epoch().count();
    }

    double Elapsed() const {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::ratio<1>> seconds = end - m_start;
        return seconds.count();
    }

private:
    std::chrono::high_resolution_clock::time_point m_start;
};
} // namespace ace

#endif