cmake_minimum_required(VERSION 4.2.2...4.2.2)

project(AnimTrainer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# this did not work with the above stated com
#set(CMAKE_CXX_MODULE_STD ON)

# on my machine:
#[[
-- CMake version: 4.2.2
-- C++ Standard: 23
-- Compiler Vendor: MSVC
-- Compiler Version: 19.50.35729.0
-- Compiler Path: C:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/MSVC/14.50.35717/bin/Hostx64/x64/cl.exe
]]
message(STATUS "CMake version: ${CMAKE_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler Vendor: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Compiler Version: ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Compiler Path: ${CMAKE_CXX_COMPILER}")

add_executable(AnimTrainer)

target_sources(AnimTrainer PRIVATE
        main.cpp
)

# quick hack: this was the only way I know how to get import std to work...
# Probably should be setting env vars or some other variable but this will due for now
target_sources(AnimTrainer PRIVATE
        FILE_SET mods
        TYPE CXX_MODULES
        BASE_DIRS
            "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Tools\\MSVC\\14.50.35717\\modules"
        FILES
            "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Tools\\MSVC\\14.50.35717\\modules\\std.ixx"
)
