# Tower-Script
A tower light PLC for Mach4 or MachPro using RGB addressable LED's. 



3 outputs are needed and an arduino. 
I used a Arduino mini pro so lots of IO for this. 
Watch your power usage though. LED strips take quite alot of power about 400ma for a strip that's 144 LED's
Edit the ino and select brightness as well as number of led's. 

 Operator->Edit Screen
 Select the very top left item in the dropdown for the Screen Manager
 Under Properties select events.
 Edit the PLC script and add the following 4 lines

```
if towerscript == nil then
	towerscript = require "towerscript" -- Load's towerscript.lua from a path. probably C:\Mach4\Modules
end
towerscript.PLCScript()
```

goto your Mach Devices page (ESS) and setup 3 output pins and enable those outputs ( output 5, 6, 7 for me )
Then within Mach4/MachPro settings, setup those outputs as well.


 
