#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <string_view>

// Per-timer statistics. "current_*" is filled during the frame that is being
// rendered; the other fields describe the last completed frame.
struct TimerStats {
    double   current_seconds = 0.0;
    double   last_seconds    = 0.0;
    double   avg_seconds     = 0.0;
    double   total_seconds   = 0.0;
    uint64_t current_calls   = 0;
    uint64_t last_calls      = 0;
    uint64_t frame_count     = 0;
};

namespace timers {
    // Access the global (single-threaded) timer registry.
    inline std::map<std::string_view, TimerStats>& Registry() {
        static std::map<std::string_view, TimerStats> registry;
        return registry;
    }

    inline void Add(std::string_view name, double seconds) {
        TimerStats& stats = Registry()[name];
        stats.current_seconds += seconds;
        stats.current_calls++;
    }

    // Call once at the end of every rendered frame.
    inline void EndFrame() {
        for (auto& [name, stats] : Registry()) {
            stats.last_seconds = stats.current_seconds;
            stats.last_calls   = stats.current_calls;
            stats.total_seconds += stats.current_seconds;
            stats.frame_count++;
            stats.avg_seconds = stats.total_seconds / static_cast<double>(stats.frame_count);

            stats.current_seconds = 0.0;
            stats.current_calls   = 0;
        }
    }

    inline const std::map<std::string_view, TimerStats>& Entries() {
        return Registry();
    }

    inline void Reset() {
        Registry().clear();
    }
}

// RAII scope timer. Add one at the start of a block:
//
//     { Timer t("Physics"); UpdatePhysics(); }
//
// The elapsed time is accumulated into the named timer when the scope ends.
class Timer {
public:
    explicit Timer(std::string_view name)
        : m_name(name), m_start(Clock::now()) {}

    ~Timer() {
        Stop();
    }

    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;

    void Stop() {
        if (m_stopped) {
            return;
        }
        m_stopped = true;

        const auto end = Clock::now();
        timers::Add(m_name, std::chrono::duration<double>(end - m_start).count());
    }

private:
    using Clock = std::chrono::steady_clock;

    std::string_view m_name;
    Clock::time_point m_start;
    bool m_stopped = false;
};
