# The Roblox To Godot Project

A GDExtension that adds [Luau](https://luau-lang.org) and creates a `RobloxVMInstance` for Godot to be able to run Roblox games.
*(+ some extras)*

About
-----
This project is made for the developers that have gotten sick of using Roblox and want to become indie. Simply convert your game without modifying any scripts and it will all work as expected

Features
--------
- Implementation of a Roblox VM that runs Luau and the task scheduler as needed.
- TODO: Implementation of Instances, Roblox data types
- TODO: Implementation of Actors

Compiling
------------
- Clone the repo
- Run `scons platform=<platform>`
- [A test project is included in the repo](https://github.com/RadiantUwU/RobloxToGodotProject/tree/master/demo)
  (binaries are in `res://addons/gdluau/bin`)

**Special thanks**
------
- https://github.com/Manonox/GDLuau
- https://github.com/WeaselGames/godot_luaAPI
