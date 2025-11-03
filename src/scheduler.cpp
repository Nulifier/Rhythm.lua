#include "scheduler.hpp"

#include <algorithm>
#include <thread>

Scheduler::TaskId Scheduler::scheduleAt(const TimePoint& time,
										const TaskFn& func,
										const TaskFn cleanup) {
	// Create the task
	Task task;
	task.id = m_nextId++;
	task.func = std::move(func);
	task.cleanup = std::move(cleanup);
	task.interval = DurationMs(0);
	task.nextRun = time;
	task.skipIfLate = false;
	task.active = true;

	// Add to the list
	m_tasks.push_back(std::move(task));

	// Update next task time
	if (time < m_nextTaskTime) {
		m_nextTaskTime = time;
	}

	return task.id;
}

Scheduler::TaskId Scheduler::scheduleAfter(const DurationMs& delay,
										   const TaskFn& func,
										   const TaskFn cleanup) {
	return scheduleAt(Clock::now() + delay, std::move(func),
					  std::move(cleanup));
}

Scheduler::TaskId Scheduler::scheduleEvery(const DurationMs& interval,
										   const TaskFn& func,
										   const TaskFn cleanup,
										   bool runImmediately,
										   bool skipIfLate) {
	// Create the task
	Task task;
	task.id = m_nextId++;
	task.func = std::move(func);
	task.cleanup = std::move(cleanup);
	task.interval = interval;
	task.nextRun = runImmediately ? Clock::now() : Clock::now() + interval;
	task.skipIfLate = skipIfLate;
	task.active = true;

	// Add to the list
	m_tasks.push_back(std::move(task));

	// Update next task time
	if (task.nextRun < m_nextTaskTime) {
		m_nextTaskTime = task.nextRun;
	}

	return task.id;
}

bool Scheduler::cancelTask(TaskId id) {
	// Find the task and mark it as inactive
	auto it = std::find_if(m_tasks.begin(), m_tasks.end(),
						   [id](const Task& task) { return task.id == id; });
	if (it != m_tasks.end()) {
		// Mark the task as inactive
		it->active = false;

		// Call cleanup function if provided
		if (it->cleanup) {
			it->cleanup(it->id);
		}
		return true;
	}
	return false;
}

void Scheduler::tick() {
	auto now = Clock::now();
	m_nextTaskTime = TimePoint::max();

	for (Task& task : m_tasks) {
		// Skip inactive tasks
		if (!task.active)
			continue;

		// Check if it's time to run the task
		if (task.nextRun <= now) {
#ifdef RHYTHM_SCHEDULER_METRICS
			// Measure start time
			auto start = Clock::now();

			// Consider a task run as "late" if it starts significantly after
			// its scheduled time
			bool wasLate = start > task.nextRun + LateThreshold;
#endif	// RHYTHM_SCHEDULER_METRICS

			// Execute the task
			task.func(task.id);

			//

#ifdef RHYTHM_SCHEDULER_METRICS
			// Measure run duration and record metrics
			auto end = Clock::now();
			auto runDuration =
				std::chrono::duration_cast<DurationMs>(end - start);
			noteTaskRun(runDuration, wasLate);
#endif	// RHYTHM_SCHEDULER_METRICS

			if (task.interval.count() > 0) {
				// Reschedule recurring task
				if (task.skipIfLate) {
					// Skip missed runs
					while (task.nextRun <= now) {
						task.nextRun += task.interval;
					}
				} else {
					// Schedule for the next interval
					task.nextRun += task.interval;
				}
			} else {
				// One-shot task, deactivate it
				task.active = false;

				// Call cleanup function if provided
				if (task.cleanup) {
					task.cleanup(task.id);
				}
			}
		}

		// Update next task time if the task is still active
		if (task.active && task.nextRun < m_nextTaskTime) {
			m_nextTaskTime = task.nextRun;
		}
	}

	// Remove inactive tasks
	m_tasks.erase(std::remove_if(m_tasks.begin(), m_tasks.end(),
								 [](const Task& task) { return !task.active; }),
				  m_tasks.end());

	// If no tasks are left, set next task time to max
	if (m_tasks.empty()) {
		m_nextTaskTime = TimePoint::max();
	}
}

bool Scheduler::loop() {
	m_running = true;
	while (m_running) {
		tick();

		// Determine when to wake up next
		auto wakeTime = nextTaskTime();

		if (wakeTime) {
			// Sleep until the next task time
			std::this_thread::sleep_until(*wakeTime);
		} else {
			// No tasks scheduled, sleep for a short duration
			std::this_thread::sleep_for(DurationMs(100));
			break;	// Exit loop if no tasks are scheduled
		}
	}

	return m_running;
}

std::optional<Scheduler::DurationMs> Scheduler::timeUntilNextTask() const {
	// If no tasks are scheduled, return nullopt
	if (m_nextTaskTime == TimePoint::max()) {
		return std::nullopt;
	}

	// Calculate the duration until the next task
	auto now = Clock::now();

	// If the next task time is in the past, return zero duration
	if (m_nextTaskTime <= now) {
		return DurationMs(0);
	}

	return std::chrono::duration_cast<DurationMs>(m_nextTaskTime - now);
}

std::optional<Scheduler::TimePoint> Scheduler::nextTaskTime() const {
	if (m_nextTaskTime == TimePoint::max()) {
		return std::nullopt;
	}
	return m_nextTaskTime;
}

#ifdef RHYTHM_SCHEDULER_METRICS

Scheduler::Metrics Scheduler::getMetrics() const {
	auto now = Clock::now();

	Metrics metrics;
	metrics.totalRuns = m_totalRuns;
	metrics.lateRuns = m_lateRuns;
	metrics.totalRunTime = m_totalRunTime;
	metrics.measurementWindow =
		std::chrono::duration_cast<DurationMs>(now - m_metricsStartTime);

	return metrics;
}

void Scheduler::resetMetrics() {
	m_totalRuns = 0;
	m_lateRuns = 0;
	m_totalRunTime = DurationMs::zero();
	m_metricsStartTime = Clock::now();
}

void Scheduler::noteTaskRun(const DurationMs& runTime, bool wasLate) {
	// Update total run time, guarding against overflow
	if (m_totalRuns < std::numeric_limits<unsigned int>::max()) {
		m_totalRuns++;
	}

	// Update late run count, guarding against overflow
	if (wasLate && m_lateRuns < std::numeric_limits<unsigned int>::max()) {
		m_lateRuns++;
	}

	// Accumulate total run time, guarding against overflow
	auto maxDur = DurationMs::max();
	if (maxDur - m_totalRunTime > runTime) {
		m_totalRunTime += runTime;
	} else {
		m_totalRunTime = maxDur;
	}
}

#endif	// RHYTHM_SCHEDULER_METRICS
