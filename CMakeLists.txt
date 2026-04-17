cmake_minimum_required(VERSION 4.2.2...4.2.2)

project(AnimTrainer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
message(STATUS "Source Dir: ${CMAKE_CURRENT_SOURCE_DIR}")

message(STATUS "Fetching GLM")
include(FetchContent)
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG        1.0.1  # You can update this to the latest tag
)
FetchContent_MakeAvailable(glm)

add_executable(AnimTrainer)

add_subdirectory(raylib)
target_link_libraries(AnimTrainer PRIVATE raylib)


target_include_directories(AnimTrainer PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_include_directories(AnimTrainer PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/raygui/src")
target_include_directories(AnimTrainer PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/random")

# each module will have its own subdir under src...
# I call this module here, buts its not a C++ module. This is just a standard link target..
target_link_libraries(AnimTrainer PRIVATE glm::glm)
add_subdirectory(src/camera)
target_link_libraries(AnimTrainer PRIVATE Camera)
add_subdirectory(src/math)
target_link_libraries(AnimTrainer PRIVATE ATMath)


target_sources(AnimTrainer PRIVATE
        src/main.cpp
        src/random/ATRandom.cpp
)
