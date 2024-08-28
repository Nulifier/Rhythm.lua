--- @meta

local rhythm = {}

--- @alias TaskFn fun(taskId: integer): any

--- @alias TaskId integer

--- Schedule a one-shot task to run at a specific time.
--- @param time integer Time to run the task, as returned by os.time().
--- @param fn TaskFn The task function to execute.
--- @return TaskId The ID of the scheduled task.
function rhythm.schedule_at(time, fn) end

--- Schedule a one-shot task to run after a delay.
--- @param delayMs integer Delay in milliseconds before running the task.
--- @param fn TaskFn The task function to execute.
--- @return TaskId The ID of the scheduled task.
function rhythm.schedule_after(delayMs, fn) end

--- Schedule a recurring task at the given itnerval.
--- @param intervalMs integer Interval in milliseconds between task executions.
--- @param fn TaskFn The task function to execute.
--- @return TaskId taskId The ID of the scheduled task.
function rhythm.schedule_every(intervalMs, fn) end

--- Cancel a scheduled task.
--- @param taskId TaskId
--- @return boolean True if the task was found and cancelled, false otherwise.
function rhythm.cancel(taskId) end

--- Runs one iteration of the scheduler, executing any tasks that are due.
--- @return nil
function rhythm.tick() end

--- Starts the scheduler loop, which continuously runs the scheduler.
--- This function should be called in a while loop in the main thread. This
--- allows it to return to Lua regularly to handle interrupts. It will block
--- until `rhythm.stop_loop()` is called or there are no scheduled tasks.
---
--- Example:
--- ```lua
--- while rhythm.loop() do end
--- ```
--- @return boolean True if the loop hasn't been stopped, false if it was stopped.
--- @see rhythm.stop_loop
function rhythm.loop() end

--- Stops the scheduler loop started by `rhythm.loop()`.
--- @return nil
function rhythm.stop_loop() end

--- Gets the milliseconds until the next scheduled task.
--- @return integer|nil The milliseconds until the next task, or nil if no tasks are scheduled.
function rhythm.get_ms_until_next_task() end

--- Gets the time of the next scheduled task, as returned by os.time().
--- @return integer|nil The time of the next task, or nil if no tasks are scheduled.
function rhythm.get_next_task_time() end

--- Gets the count of currently scheduled tasks.
--- @return integer
function rhythm.get_task_count() end

--- @alias SchedulerMetrics { totalRuns: integer, lateRuns: integer, totalRunTimeMs: integer, measurementWindowMs: integer, runTimeFraction: number }

--- Gets metrics about the scheduler's performance.
--- If RHYTHM_SCHEDULER_METRICS is not enabled, this function returns nil.
--- @return SchedulerMetrics|nil
function rhythm.get_scheduler_metrics() end

--- Resets the scheduler metrics collected by `rhythm.get_scheduler_metrics()`.
--- If RHYTHM_SCHEDULER_METRICS is not enabled, this function does nothing.
--- @return nil
function rhythm.reset_scheduler_metrics() end

return rhythm
