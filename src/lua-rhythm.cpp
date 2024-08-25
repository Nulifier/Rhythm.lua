#include "lua-rhythm.h"
#include "chrono-utils.hpp"
#include "lauxlib.h"
#include "lua-rhythm-private.hpp"

#ifdef RHYTHM_STACK_CHECK
#define STACK_START(fn_name, nargs)                         \
	int rhythm_stack_top_##fn_name = lua_gettop(L) - nargs; \
	enum {}
#define STACK_END(fn_name, nresults)                                         \
	if (lua_gettop(L) != rhythm_stack_top_##fn_name + nresults) {            \
		fprintf(                                                             \
			stderr, "Stack imbalance in %s: expected %d results, got %d\n",  \
			#fn_name, nresults, lua_gettop(L) - rhythm_stack_top_##fn_name); \
	}                                                                        \
	enum {}
#else
#define STACK_START(name, nargs) enum {}
#define STACK_END(name, nresults) enum {}
#endif

static const char* RHYTHM_SCHEDULER_UDATA = "rhythm.scheduler";
static const char* RHYTHM_SCHEDULER_METATABLE = "rhythm.scheduler_meta";

const luaL_Reg rhythm_funcs[] = {
	{"schedule_at", lua_schedule_at},
	{"schedule_after", lua_schedule_after},
	{"schedule_every", lua_schedule_every},
	{"cancel_task", lua_cancel_task},
	{"tick", lua_tick},
	{"loop", lua_loop},
	{"stop_loop", lua_stop_loop},
	{"ms_until_next_task", lua_get_ms_until_next_task},
	{"get_next_task_time", lua_get_next_task_time},
	{"get_task_count", lua_get_task_count},
	{"get_scheduler_metrics", lua_get_scheduler_metrics},
	{"reset_scheduler_metrics", lua_reset_scheduler_metrics},
	{NULL, NULL}  // Sentinel
};

int luaopen_rhythm(lua_State* L) {
	STACK_START(luaopen_rhythm, 1);

	// Get the module name from the first argument, popping it after
	const char* modName = luaL_checkstring(L, 1);
	lua_pop(L, 1);

	// Create the module table
	lua_newtable(L);

	luaL_register(L, nullptr, rhythm_funcs);

	STACK_END(luaopen_rhythm, 1);

	return 1;
}

void lua_pop_extra_args(lua_State* L, int expected) {
	int actual = lua_gettop(L);
	if (actual > expected) {
		lua_pop(L, actual - expected);
	}
}

Scheduler& lua_get_scheduler(lua_State* L) {
	STACK_START(lua_get_scheduler, 0);

	// Get the scheduler from the registry if it exists
	lua_getfield(L, LUA_REGISTRYINDEX, RHYTHM_SCHEDULER_UDATA);
	if (lua_isuserdata(L, -1)) {
		// Found, retrieve the pointer
		auto scheduler = *static_cast<Scheduler**>(lua_touserdata(L, -1));
		lua_pop(L, 1);	// Pop the user data

		STACK_END(lua_get_scheduler, 0);
		return *scheduler;
	} else {
		// Not found, pop the nil value
		lua_pop(L, 1);

		// Create the user data to hold the scheduler
		void** udata = static_cast<void**>(lua_newuserdata(L, sizeof(void*)));

		// Create the scheduler and store it in the user data
		*udata = new Scheduler();

		// Create the metatable for the scheduler
		if (luaL_newmetatable(L, RHYTHM_SCHEDULER_METATABLE)) {
			// It only needs a __gc method for cleanup
			lua_pushcfunction(L, [](lua_State* L) -> int {
				// Get the scheduler instance
				// We can convert it to a pointer directly since the metatable
				// is only assigned to the scheduler user data
				auto* scheduler = &lua_get_scheduler(L);
				delete scheduler;

				return 0;
			});

			lua_setfield(L, -2, "__gc");
		}

		lua_setmetatable(L, -2);

		// Store the user data in the registry
		lua_setfield(L, LUA_REGISTRYINDEX, RHYTHM_SCHEDULER_UDATA);

		STACK_END(lua_get_scheduler, 0);
		return *static_cast<Scheduler*>(*udata);
	}
}

void call_lua_task_function(lua_State* L, int funcRef, Scheduler::TaskId id) {
	STACK_START(scheduled_task, 0);

	// Push the error function
	lua_push_error_func(L);

	// Push the function onto the stack
	lua_rawgeti(L, LUA_REGISTRYINDEX, funcRef);

	// Push the task id as the first argument
	lua_pushinteger(L, id);

	if (lua_pcall(L, 1, 0, -3) != 0) {
		const char* err = lua_tostring(L, -1);
		fprintf(stderr, "Error in scheduled task: %s\n", err);
		lua_pop(L, 1);	// Pop error message
	}

	// Remove the error function from the stack
	lua_pop(L, 1);

	STACK_END(scheduled_task, 0);
}

void removee_lua_task_function(lua_State* L, int funcRef) {
	STACK_START(cleanup_func, 0);

	// Cleanup function to remove the function reference
	luaL_unref(L, LUA_REGISTRYINDEX, funcRef);

	STACK_END(cleanup_func, 0);
}

void lua_push_error_func(lua_State* L) {
	STACK_START(lua_push_error_func, 0);

	// Push debug.traceback function
	lua_getglobal(L, "debug");
	lua_getfield(L, -1, "traceback");
	lua_remove(L, -2);	// Remove 'debug' table, leaving only the function

	STACK_END(lua_push_error_func, 1);
}

int lua_schedule_at(lua_State* L) {
	// Remove any extra arguments beyond the expected two
	lua_pop_extra_args(L, 2);

	STACK_START(lua_schedule_at, 2);

	// Get the time (as time_t)
	std::time_t time = static_cast<std::time_t>(luaL_checkinteger(L, 1));
	Scheduler::TimePoint tp = chrono_utils::time_t_to_steady(time);

	// Store the function as a ref in the registry and get its reference ID
	int funcRef = luaL_ref(L, LUA_REGISTRYINDEX);

	// Pop the time (Stack should be empty now)
	lua_pop(L, 1);

	// Schedule the task
	Scheduler& scheduler = lua_get_scheduler(L);
	Scheduler::TaskId taskId = scheduler.scheduleAt(
		tp,
		[L, funcRef](Scheduler::TaskId id) {
			call_lua_task_function(L, funcRef, id);
		},
		[L, funcRef](Scheduler::TaskId id) {
			removee_lua_task_function(L, funcRef);
		});

	// Return the task ID
	lua_pushinteger(L, taskId);

	STACK_END(lua_schedule_at, 1);

	return 1;
}

int lua_schedule_after(lua_State* L) {
	lua_pop_extra_args(L, 2);

	STACK_START(lua_schedule_after, 2);

	// Get the delay in milliseconds
	lua_Integer delayMs = luaL_checkinteger(L, 1);
	if (delayMs < 0) {
		luaL_error(L, "Delay must be non-negative");
	}
	Scheduler::DurationMs tp(delayMs);

	// Store the function as a ref in the registry and get its reference ID
	int funcRef = luaL_ref(L, LUA_REGISTRYINDEX);

	// Pop the delay (Stack should be empty now)
	lua_pop(L, 1);

	// Schedule the task
	Scheduler& scheduler = lua_get_scheduler(L);
	Scheduler::TaskId taskId = scheduler.scheduleAfter(
		tp,
		[L, funcRef](Scheduler::TaskId id) {
			call_lua_task_function(L, funcRef, id);
		},
		[L, funcRef](Scheduler::TaskId id) {
			removee_lua_task_function(L, funcRef);
		});

	// Return the task ID
	lua_pushinteger(L, taskId);

	STACK_END(lua_schedule_after, 1);

	return 1;
}

int lua_schedule_every(lua_State* L) {
	lua_pop_extra_args(L, 2);

	STACK_START(lua_schedule_every, 2);

	// Get the delay in milliseconds
	lua_Integer delayMs = luaL_checkinteger(L, 1);
	if (delayMs < 0) {
		luaL_error(L, "Delay must be non-negative");
	}
	Scheduler::DurationMs tp(delayMs);

	// Store the function as a ref in the registry and get its reference ID
	int funcRef = luaL_ref(L, LUA_REGISTRYINDEX);

	// Pop the delay (Stack should be empty now)
	lua_pop(L, 1);

	// Schedule the task
	Scheduler& scheduler = lua_get_scheduler(L);
	Scheduler::TaskId taskId = scheduler.scheduleEvery(
		tp,
		[L, funcRef](Scheduler::TaskId id) {
			call_lua_task_function(L, funcRef, id);
		},
		[L, funcRef](Scheduler::TaskId id) {
			removee_lua_task_function(L, funcRef);
		});

	// Return the task ID
	lua_pushinteger(L, taskId);

	STACK_END(lua_schedule_every, 1);

	return 1;
}

int lua_cancel_task(lua_State* L) {
	lua_pop_extra_args(L, 1);

	STACK_START(lua_cancel_task, 1);

	// Get the task ID
	Scheduler::TaskId taskId =
		static_cast<Scheduler::TaskId>(luaL_checkinteger(L, 1));
	lua_pop(L, 1);

	// Cancel the task
	Scheduler& scheduler = lua_get_scheduler(L);
	bool success = scheduler.cancelTask(taskId);

	lua_pushboolean(L, success);

	STACK_END(lua_cancel_task, 1);

	return 1;
}

int lua_tick(lua_State* L) {
	lua_pop_extra_args(L, 0);

	STACK_START(lua_tick, 0);

	Scheduler& scheduler = lua_get_scheduler(L);
	scheduler.tick();

	STACK_END(lua_tick, 0);

	return 0;
}

int lua_loop(lua_State* L) {
	lua_pop_extra_args(L, 0);

	STACK_START(lua_loop, 0);

	Scheduler& scheduler = lua_get_scheduler(L);
	bool keepRunning = scheduler.loop();

	lua_pushboolean(L, keepRunning);

	STACK_END(lua_loop, 1);

	return 1;
}

int lua_stop_loop(lua_State* L) {
	lua_pop_extra_args(L, 0);

	STACK_START(lua_stop_loop, 0);

	Scheduler& scheduler = lua_get_scheduler(L);
	scheduler.stopLoop();

	STACK_END(lua_stop_loop, 0);

	return 0;
}

int lua_get_ms_until_next_task(lua_State* L) {
	lua_pop_extra_args(L, 0);

	STACK_START(lua_get_ms_until_next_task, 0);

	Scheduler& scheduler = lua_get_scheduler(L);
	auto msOpt = scheduler.timeUntilNextTask();
	if (msOpt) {
		lua_pushinteger(L, msOpt->count());
	} else {
		lua_pushnil(L);
	}

	STACK_END(lua_get_ms_until_next_task, 1);

	return 0;
}

int lua_get_next_task_time(lua_State* L) {
	lua_pop_extra_args(L, 0);

	STACK_START(lua_get_next_task_time, 0);

	Scheduler& scheduler = lua_get_scheduler(L);
	auto tpOpt = scheduler.nextTaskTime();
	if (tpOpt) {
		std::time_t t = chrono_utils::steady_to_time_t(*tpOpt);
		lua_pushinteger(L, static_cast<lua_Integer>(t));
	} else {
		lua_pushnil(L);
	}

	STACK_END(lua_get_next_task_time, 1);

	return 1;
}

int lua_get_task_count(lua_State* L) {
	lua_pop_extra_args(L, 0);

	STACK_START(lua_get_task_count, 0);

	Scheduler& scheduler = lua_get_scheduler(L);
	lua_pushinteger(L, static_cast<lua_Integer>(scheduler.taskCount()));

	STACK_END(lua_get_task_count, 1);

	return 1;
}

int lua_get_scheduler_metrics(lua_State* L) {
	lua_pop_extra_args(L, 0);

	STACK_START(lua_get_scheduler_metrics, 0);
#ifdef RHYTHM_SCHEDULER_METRICS
	Scheduler& scheduler = lua_get_scheduler(L);
	Scheduler::Metrics metrics = scheduler.getMetrics();

	lua_newtable(L);
	lua_pushinteger(L, metrics.totalRuns);
	lua_setfield(L, -2, "totalRuns");
	lua_pushinteger(L, metrics.lateRuns);
	lua_setfield(L, -2, "lateRuns");
	lua_pushinteger(L, metrics.totalRunTime.count());
	lua_setfield(L, -2, "totalRunTimeMs");
	lua_pushinteger(L, metrics.measurementWindow.count());
	lua_setfield(L, -2, "measurementWindowMs");
	lua_pushnumber(L, metrics.runTimeFraction());
	lua_setfield(L, -2, "runTimeFraction");
#else
	// Metrics not enabled, return nil
	lua_pushnil(L);
#endif	// RHYTHM_SCHEDULER_METRICS

	STACK_END(lua_get_scheduler_metrics, 1);

	return 1;
}

int lua_reset_scheduler_metrics(lua_State* L) {
	lua_pop_extra_args(L, 0);

	STACK_START(lua_reset_scheduler_metrics, 0);

#ifdef RHYTHM_SCHEDULER_METRICS
	Scheduler& scheduler = lua_get_scheduler(L);
	scheduler.resetMetrics();
#endif	// RHYTHM_SCHEDULER_METRICS

	STACK_END(lua_reset_scheduler_metrics, 0);

	return 0;
}
