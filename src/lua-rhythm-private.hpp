#pragma once

#include <lua.h>
#include "scheduler.hpp"

void lua_pop_extra_args(lua_State* L, int expected);

/**
 * Retrieves the Scheduler instance from the Lua registry.
 * If it doesn't exist, it creates a new one, stores it in the registry, and
 * returns it.
 */
Scheduler& lua_get_scheduler(lua_State* L);

void call_lua_task_function(lua_State* L, int funcRef, Scheduler::TaskId id);
void removee_lua_task_function(lua_State* L, int funcRef);

void lua_push_error_func(lua_State* L);

int lua_schedule_at(lua_State* L);
int lua_schedule_after(lua_State* L);
int lua_schedule_every(lua_State* L);
int lua_cancel_task(lua_State* L);
int lua_tick(lua_State* L);
int lua_loop(lua_State* L);
int lua_stop_loop(lua_State* L);
int lua_get_ms_until_next_task(lua_State* L);
int lua_get_next_task_time(lua_State* L);
int lua_get_task_count(lua_State* L);

int lua_get_scheduler_metrics(lua_State* L);
int lua_reset_scheduler_metrics(lua_State* L);
