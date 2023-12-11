#!/usr/bin/env python
import os
import sys

env = SConscript("godot-cpp/SConstruct")
lua_env = env.Clone()
module_env = env.Clone()
hllc_env = env.Clone()

if env["target"] == "template_debug":
    lua_env.Append(CCFLAGS=["-g"])
    module_env.Append(CCFLAGS=["-g"])
    hllc_env.Append(CCFLAGS=["-g"])

if lua_env["platform"] == "linux":
    lua_env.Append(CPPDEFINES=["LUA_USE_POSIX"])
elif lua_env["platform"] == "ios":
    lua_env.Append(CPPDEFINES=["LUA_USE_IOS"])

lua_env.Append(CPPDEFINES = ["MAKE_LIB"])
lua_env.Append(CXXFLAGS = ["-std=c++17"])
lua_env.Append(CFLAGS = ["-std=c99"])
hllc_env.Append(CPPDEFINES = ["MAKE_LIB","USE_GODOT"])
hllc_env.Append(CXXFLAGS = ["-std=c++20"])
hllc_env.Append(CFLAGS = ["-std=c99"])

module_env.Append(CXXFLAGS = ["-Wno-non-pod-varargs"])

cpp_paths = [Dir("src").abspath, Dir("/").abspath, Dir("src/classes/roblox").abspath]
hllc_cpp_paths = []
lua_cpp_paths = []

luau_paths = [
    "Common",
    "Ast",
    "Compiler",
    "CodeGen",
    "Analysis",
    "VM",
    #"extern/isocline",
]
hllc_paths = [
    "api",
    "core"
]

luau_include_paths = [os.path.join("luau", x, "include") for x in luau_paths]
luau_source_paths = [os.path.join("luau", x, "src") for x in luau_paths]
hllc_include_paths = [os.path.join("hllc", x, "include") for x in hllc_paths]
hllc_source_paths = [os.path.join("hllc", x, "src") for x in hllc_paths]
lua_cpp_paths.extend(luau_include_paths)
lua_cpp_paths.extend(luau_source_paths)
cpp_paths.extend(luau_include_paths)
cpp_paths.extend(hllc_include_paths)
hllc_cpp_paths.extend(luau_include_paths)
hllc_cpp_paths.extend(hllc_include_paths)
hllc_cpp_paths.extend(hllc_source_paths)

lua_env.AppendUnique(CPPPATH=lua_cpp_paths, delete_existing=True)
module_env.Append(CPPPATH=cpp_paths)
hllc_env.Append(CPPPATH=hllc_cpp_paths, delete_existing=True)

lua_sources = []
for path in luau_include_paths: 
    lua_sources.extend(Glob(path + "/*.hpp"))
    lua_sources.extend(Glob(path + "/*.h"))
    lua_sources.extend(Glob(path + "/*.cpp"))
    lua_sources.extend(Glob(path + "/*.c"))

for path in luau_source_paths:
    lua_sources.extend(Glob(path + "/*.hpp"))
    lua_sources.extend(Glob(path + "/*.h"))
    lua_sources.extend(Glob(path + "/*.cpp"))
    lua_sources.extend(Glob(path + "/*.c"))

hllc_sources = []
for path in hllc_include_paths: 
    hllc_sources.extend(Glob(path + "/*.hpp"))
    hllc_sources.extend(Glob(path + "/*.h"))
    hllc_sources.extend(Glob(path + "/*.cpp"))
    hllc_sources.extend(Glob(path + "/*.c"))

for path in hllc_source_paths:
    hllc_sources.extend(Glob(path + "/*.hpp"))
    hllc_sources.extend(Glob(path + "/*.h"))
    hllc_sources.extend(Glob(path + "/*.cpp"))
    hllc_sources.extend(Glob(path + "/*.c"))

luau_library_name = "RobloxToGodotProject_luau{}{}".format(env['suffix'], env["LIBSUFFIX"])
luau_library = lua_env.StaticLibrary("bin/{}".format(luau_library_name), source=lua_sources)

hllc_env.Default(luau_library)
hllc_library_name = "RobloxToGodotProject_hllc{}{}".format(env['suffix'], env["LIBSUFFIX"])
hllc_library = hllc_env.StaticLibrary("bin/{}".format(hllc_library_name), source=hllc_sources)

module_env.Default(luau_library,hllc_library)
module_env.Append(LIBPATH=[Dir("bin").abspath])
module_env.Append(LIBS=[luau_library_name,hllc_library_name])

sources = Glob("*.cpp")
sources.append(Glob("src/*.cpp"))
sources.append(Glob("src/classes/*.cpp"))
sources.append(Glob("src/classes/roblox/core/*.cpp"))

env["suffix"] = env["suffix"].replace(".dev", "").replace(".double", "").replace(".simulator", "")


if env["platform"] == "macos":
    library = module_env.SharedLibrary(
        "demo/addons/RobloxToGodotProject/bin/libRobloxToGodotProject.{}.{}.framework/libRobloxToGodotProject.{}.{}".format(
            env["platform"], env["target"], env["platform"], env["target"]
        ),
        source=sources,
    )
else:
    library = module_env.SharedLibrary(
        "demo/addons/RobloxToGodotProject/bin/libRobloxToGodotProject{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )

env.Default(library)
