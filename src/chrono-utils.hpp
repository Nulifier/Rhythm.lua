#pragma once

#include <chrono>

namespace chrono_utils {
using SteadyClock = std::chrono::steady_clock;
using SystemClock = std::chrono::system_clock;
using SteadyTimePoint = std::chrono::steady_clock::time_point;
using SystemTimePoint = std::chrono::system_clock::time_point;

// Converts from std::chrono::system_clock::time_point to
// std::chrono::steady_clock::time_point
inline SteadyTimePoint time_point_system_to_steady(const SystemTimePoint& tp) {
	auto now_sys = SystemClock::now();
	auto now_steady = SteadyClock::now();

	return now_steady + (tp - now_sys);
}

// Converts from std::chrono::steady_clock::time_point to
// std::chrono::system_clock::time_point
inline SystemTimePoint time_point_steady_to_system(const SteadyTimePoint& tp) {
	auto now_sys = SystemClock::now();
	auto now_steady = SteadyClock::now();

	return now_sys + (tp - now_steady);
}

// Converts from std::time_t to std::chrono::steady_clock::time_point
inline SteadyTimePoint time_t_to_steady(std::time_t t) {
	return time_point_system_to_steady(SystemClock::from_time_t(t));
}

// Converts from std::chrono::steady_clock::time_point to std::time_t
inline std::time_t steady_to_time_t(const SteadyTimePoint& tp) {
	return SystemClock::to_time_t(time_point_steady_to_system(tp));
}

}  // namespace chrono_utils
