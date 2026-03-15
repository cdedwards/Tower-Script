--[[
    Mach4 CNC RGB LED Control Script
    Controls addressable RGB LEDs via Arduino Mini
    Communication via 3 digital outputs (3-bit binary encoding)
	Put this script in your Modules directory and then Edit your screen to add the following  4 lines to your PLC
	
	
	if towerscript == nil then
		towerscript = require "towerscript" 
	end

	towerscript.PLCScript()
	
--]]
local towerscript = {}
-- Configuration
-- Define which outputs to use (change these to match your setup)
--local  = mc.OSIG_OUTPUT5  -- LSB (Least Significant Bit)
--local OUTPUT_BIT1 = mc.OSIG_OUTPUT6  -- Middle bit
--local OUTPUT_BIT2 = mc.OSIG_OUTPUT7  -- MSB (Most Significant Bit)
local inst = mc.mcGetInstance()
local OUTPUT_BIT0 = mc.mcSignalGetHandle(inst, mc.OSIG_OUTPUT5)
local OUTPUT_BIT1 = mc.mcSignalGetHandle(inst, mc.OSIG_OUTPUT6)
local OUTPUT_BIT2 = mc.mcSignalGetHandle(inst, mc.OSIG_OUTPUT7)

-- LED States (0-7 since we have 3 bits = 8 possible states)
local LED_OFF = 0      -- Binary: 000
local LED_IDLE = 1     -- Binary: 001
local LED_RUNNING = 2  -- Binary: 010
local LED_PAUSED = 3   -- Binary: 011
local LED_ERROR = 4    -- Binary: 100
local LED_HOMING = 5   -- Binary: 101
local LED_TOOL = 6     -- Binary: 110
local LED_ESTOP = 7    -- Binary: 111 - HIGHEST PRIORITY WARNING

-- Color definitions (defined in Arduino, not sent via outputs)
local stateNames = {
    [0] = "OFF",
    [1] = "IDLE",
    [2] = "RUNNING",
    [3] = "PAUSED",
    [4] = "ERROR",
    [5] = "HOMING",
    [6] = "TOOL CHANGE",
    [7] = "E-STOP ACTIVE"}

-- Current state tracking
local currentState = LED_OFF
local lastState = -1

-- Initialize outputs
function InitializeOutputs()
    local inst = mc.mcGetInstance()
    -- Set all outputs to low initially
    mc.mcSignalSetState(OUTPUT_BIT0, 0)
    mc.mcSignalSetState(OUTPUT_BIT1, 0)
    mc.mcSignalSetState(OUTPUT_BIT2, 0)
end

-- Send state to Arduino via 3-bit binary encoding
function SetOutputState(state)
    -- Ensure state is within valid range (0-7)
    if state < 0 then state = 0 end
    if state > 7 then state = 7 end
    
    -- Extract individual bits
    local bit0 = state % 2           -- LSB
    local bit1 = math.floor(state / 2) % 2
    local bit2 = math.floor(state / 4) % 2  -- MSB
    
    -- Set the outputs
    mc.mcSignalSetState(OUTPUT_BIT0, bit0)
    mc.mcSignalSetState(OUTPUT_BIT1, bit1)
    mc.mcSignalSetState(OUTPUT_BIT2, bit2)
    
    currentState = state
end

-- Set LED state based on machine state
function SetLEDState(state)
 --   if state ~= currentState then
        SetOutputState(state)
--        lastState = currentState
--    end
end

-- PLC Script - Monitor machine state and update LEDs
function towerscript.PLCScript()
    local inst = mc.mcGetInstance()
	
    -- Get machine state
    local state,rc = mc.mcCntlGetState(inst)
    
    -- Mach4 State Constants (correct values):
    -- mc.MC_STATE_IDLE = 0
    -- mc.MC_STATE_RUN = 1
    -- mc.MC_STATE_HOLD = 10
    -- mc.MC_STATE_JOG = 2
    -- mc.MC_STATE_MRUN = 5
    
    -- Check if machine is enabled
    local idle = 1
	idle = (state == mc.MC_STATE_IDLE)
	
	local enabled = mc.mcSignalGetState (mc.mcSignalGetHandle (inst, mc.OSIG_MACHINE_ENABLED))

    -- Check if cycle is running
	local isRunning = mc.mcSignalGetState(mc.mcSignalGetHandle(inst, mc.OSIG_RUNNING_GCODE))

    -- Check if machine is homing
   local isHoming = mc.mcAxisIsHoming(inst, mc.X_AXIS)
		 isHoming = isHoming + mc.mcAxisIsHoming(inst, mc.Y_AXIS)
		 isHoming = isHoming + mc.mcAxisIsHoming(inst, mc.Z_AXIS)
		 isHoming = isHoming + mc.mcAxisIsHoming(inst, mc.A_AXIS)
--	local isHoming, i
--	for i=0, 11 do 
--		local h
--		local rc
--		h, rc = mc.mcAxisIsHoming(inst, i)
--		isHoming= isHoming + h
--	end
	if (isHoming ~= 0) then
		isHoming = 1
	end
	
    -- Check for E-Stop (HIGHEST PRIORITY)
    local estopActive,rc = mc.mcSignalGetState(mc.mcSignalGetHandle(inst, mc.ISIG_EMERGENCY))

	
    local istoolChange = mc.mcSignalGetState(mc.mcSignalGetHandle(inst, mc.OSIG_TOOL_CHANGE))
	local isProbing = mc.mcSignalGetState(mc.mcSignalGetHandle(inst, mc.OSIG_MACHINE_PROBING))
	local isFeedHold = mc.mcSignalGetState(mc.mcSignalGetHandle(inst, mc.OSIG_FEEDHOLD))
	
    -- Determine LED state based on machine condition (PRIORITY ORDER - E-STOP FIRST!)
    local newState = LED_OFF
    local LED_OFF = 0      -- Binary: 000
-- LED_IDLE = 1     -- Binary: 001
-- LED_RUNNING = 2  -- Binary: 010
-- LED_PAUSED = 3   -- Binary: 011
-- LED_ERROR = 4    -- Binary: 100
-- LED_HOMING = 5   -- Binary: 101
-- LED_TOOL = 6     -- Binary: 110
-- LED_ESTOP = 7    -- Binary: 111 - HIGHEST PRIORITY WARNING

    if estopActive == 1 then
        -- E-STOP has absolute priority - overrides everything!
        newState = LED_ESTOP
    elseif enabled == 0 then
        newState = LED_ERROR
    elseif isHoming == 1  then
        newState = LED_HOMING
    elseif isFeedHold == 1 then --state == mc.MC_STATE_HOLD then
        newState = LED_PAUSED		
    elseif isRunning == 1 then
        newState = LED_RUNNING
	elseif istoolChange == 1 then
		newState = LED_TOOL
	elseif enabled == 1 then
        newState = LED_IDLE
    else
        newState = LED_OFF
    end
	
    if (newState == lastState) then
		return
	end
	lastState = newState
	
    -- Update LED state only if changed
    SetLEDState(newState)
end

-- Screen button functions
function ButtonLEDsOn()
    SetLEDState(LED_IDLE)
end

function ButtonLEDsOff()
    SetLEDState(LED_OFF)
end

function ButtonSetState(state)
    -- Manually set a specific state (0-7)
    -- Can be called from screen buttons with parameter
    if state >= 0 and state <= 7 then
        SetLEDState(state)
    end
end

-- Get current state name for display
function GetStateName()
    return stateNames[currentState] or "UNKNOWN"
end

-- Cleanup on script unload
function ScriptUnload()
    SetLEDState(LED_OFF)
end

InitializeOutputs()
 
 
if (mc.mcInEditor() == 1) then
	towerscript.PLCScript()
end

return towerscript
 
