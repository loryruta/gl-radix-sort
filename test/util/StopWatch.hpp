#pragma once

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <sstream>

namespace glu
{
    class StopWatch
    {
        using ClockT = std::chrono::system_clock;

    private:
        ClockT::time_point m_start;

    public:
        StopWatch() { reset(); };
        ~StopWatch() = default;

        void reset() { m_start = std::chrono::system_clock::now(); }

        ClockT::duration elapsed_time() { return std::chrono::system_clock::now() - m_start; }

        uint64_t elapsed_millis()
        {
            return std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_time()).count();
        }
        uint64_t elapsed_nanos()
        {
            return std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed_time()).count();
        }

        std::string elapsed_time_str()
        {
            uint64_t ns = elapsed_nanos();
            double ms = double(ns / 1000.0) / 1000.0;
            double s = ms / 1000.0;

            if (s >= 0.1)
            {
                std::stringstream stream; // <format> header not available with g++-12
                stream << std::fixed << std::setprecision(1) << s;
                return stream.str() + " s";
            }
            else if (ms >= 0.001)
            {
                std::stringstream stream; // <format> header not available with g++-12
                stream << std::fixed << std::setprecision(3) << ms;
                return stream.str() + " ms";
            }
            else
            {
                return std::to_string(ns) + " ns";
            }
        }
    };
} // namespace glu
