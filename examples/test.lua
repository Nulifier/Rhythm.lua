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

rhythm.schedule_every(5000, function(taskId)
	print("Should run immediately!", taskId)
end, true)

while rhythm.loop() do end

print("Ta da!")
