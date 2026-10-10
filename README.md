UnrealLua is a Lua scripting plugin for the Unreal Engine 5.

//@TODO : Documentation (sorry, takes a lot of work!)

WARNING: The current version is not yet ready for use.
Expected date for first usable version: November 2026.

Main Features:
- Plug and play, no complicated setup needed
- Run Lua scripts at game runtime
- Attach Lua scripts to any UObject to override UFunctions and modify behavior
- Attach additional Lua script values to any UObject
- GameMode dependent script loading
- High modding capabilities by dynamic script patching
- Network Replication of Lua script value attached to UObjects
- Remote procedure calls (RPCS) of Lua functions
- Overriding Enhanced Input events
- Various Blueprint Nodes for interacting with Lua runtime (tables, coroutines, calling functions, accessing values)
- In-game Lua script editor UI, allowing Lua scripting and modding in shipping builds
- A Lua-based compiler able to create new UClasses, UScriptStructs and UEnums from Lua scripts that can be used as a base for Blueprints
