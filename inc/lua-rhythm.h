#pragma once

#include <lua.h>
#include "lua-rhythm-export.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Register the Rhythm library with Lua.
 */
LUA_RHYTHM_EXPORT int luaopen_rhythm(lua_State* L);

#ifdef __cplusplus
}
#endif
