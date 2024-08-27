# Rhythm.lua

Rhythm is a small C++ library exposing a Lua API for scheduling one-shot and
recurring tasks from Lua.

It provides a simple scheduler implementation (see
[`Scheduler`](src/scheduler.hpp)) and a Lua binding in
[`src/lua-rhythm.cpp`](src/lua-rhythm.cpp).
The Lua API surface is documented in [lua-docs/rhythm.lua](lua-docs/rhythm.lua).

## Example Usage
```lua
local rhythm = require("rhythm")

local function tick(taskId)
	print("Tick!", taskId)
end

local function tock(taskId)
	print("Tock!", taskId)
end

local function schedule_tock()
	local id = rhythm.schedule_every(2000, tock)
	print("Scheduling tock every 2 seconds, id: " .. id)
end

local function stop_loop(taskId)
	print("Stopping loop")
	rhythm.stop_loop()
end

rhythm.schedule_every(2000, tick)
rhythm.schedule_after(1000, schedule_tock)
rhythm.schedule_after(10000, stop_loop)

while rhythm.loop() do end

print("Ta da!")
```

## Building
This project uses CMake to build. From the repository root:

```sh
# Create a build directory and configure
mkdir build && cd build
cmake ..	# pass -DCMAKE_BUILD_TYPE_RELEASE for a release build
            # optionally add -DRHYTHM_SCHEDULER_METRICS=OFF to disable metrics
		
# Build the shared library
cmake --build .

# The final shared library is located at build/rhythm.{so|dll}
size build/rhythm.*
```
