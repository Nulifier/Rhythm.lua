#pragma once

#include <chrono>
#include <deque>
#include <functional>
#include <optional>
#include "rhythm-config.hpp"

class Scheduler {
   public:
	using TaskId = int;
	using TaskFn = std::function<void(TaskId)>;
	using Clock = std::chrono::steady_clock;
	using TimePoint = Clock::time_point;
	using DurationMs = std::chrono::milliseconds;

	// Threshold to consider a task run as "late" (in ms)
	static constexpr DurationMs LateThreshold = DurationMs(10);

	/**
	 * Schedule a one-shot task to run at a specific time.
	 * @param time The time point to run the task.
	 * @param func The task function to execute.
	 * @param cleanup Optional cleanup function called after the task completes.
	 * @return The ID of the scheduled task.
	 */
	TaskId scheduleAt(const TimePoint& time,
					  const TaskFn& func,
					  const TaskFn cleanup = TaskFn());

	/**
	 * Schedule a one-shot task to run after a delay.
	 * @param delay The delay after which to run the task.
	 * @param func The task function to execute.
	 * @param cleanup Optional cleanup function called after the task completes.
	 * @return The ID of the scheduled task.
	 */
	TaskId scheduleAfter(const DurationMs& delay,
						 const TaskFn& func,
						 const TaskFn cleanup = TaskFn());

	/**
	 * Schedule a recurring task at the given interval.
	 * @param interval The interval at which to run the task.
	 * @param func The task function to execute.
	 * @param cleanup Optional cleanup function called after each task run.
	 * @param skipIfLate If true, skip missed runs if late.
	 * @return The ID of the scheduled task.
	 */
	TaskId scheduleEvery(const DurationMs& interval,
						 const TaskFn& func,
						 const TaskFn cleanup = TaskFn(),
						 bool skipIfLate = false);

	/**
	 * Cancel a scheduled task.
	 * @param id The ID of the task to cancel.
	 * @return True if the task was found and cancelled, false otherwise.
	 */
	bool cancelTask(TaskId id);

	void tick();
	bool loop();

	void stopLoop() { m_running = false; }

	std::optional<DurationMs> timeUntilNextTask() const;
	std::optional<TimePoint> nextTaskTime() const;

	std::size_t taskCount() const { return m_tasks.size(); }

#ifdef RHYTHM_SCHEDULER_METRICS
	struct Metrics {
		/** Total number of task runs */
		unsigned int totalRuns = 0;
		/** Number of runs that were considered late */
		unsigned int lateRuns = 0;
		/** Total accumulated run time of all tasks */
		DurationMs totalRunTime = DurationMs::zero();
		/** Elapsed time since metrics started/were reset */
		DurationMs measurementWindow = DurationMs::zero();

		/**
		 * Fraction of time spent running tasks over the measurement window.
		 * @tparam T The numeric type to return (default: double).
		 * @return The fraction of time spent running tasks (0.0 to 1.
		 */
		template <typename T = double>
		T runTimeFraction() const {
			auto denom = measurementWindow.count();
			if (denom == 0)
				return 0.0;
			return static_cast<T>(totalRunTime.count()) / static_cast<T>(denom);
		}
	};

	/**
	 * Get the current collected metrics.
	 * @return The current metrics.
	 */
	Metrics getMetrics() const;

	/**
	 * Reset the collected metrics.
	 * Clears counters and restarts the measurement window.
	 */
	void resetMetrics();

#endif	// RHYTHM_SCHEDULER_METRICS

   private:
	struct Task {
		TaskId id;
		TaskFn func;
		TaskFn cleanup;		  // Optional cleanup function
		DurationMs interval;  // Zero if one-shot
		TimePoint nextRun;
		bool skipIfLate;
		bool active;
	};

	std::deque<Task> m_tasks;
	TaskId m_nextId = 1;
	TimePoint m_nextTaskTime = TimePoint::max();
	bool m_running = false;

	// Metrics
	unsigned int m_totalRuns = 0;
	unsigned int m_lateRuns = 0;
	DurationMs m_totalRunTime = DurationMs::zero();
	Clock::time_point m_metricsStartTime = Clock::now();

	/**
	 * Internal helper to note a task run for metrics.
	 * @param runTime The duration the task took to run.
	 * @param wasLate Whether the task run was late.
	 */
	void noteTaskRun(const DurationMs& runTime, bool wasLate);
};
