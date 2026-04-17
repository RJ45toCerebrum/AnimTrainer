message(STATUS "INCLUDE: ${CMAKE_CURRENT_SOURCE_DIR}")

# Create a library for the component
add_library(Camera STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/ATCamera.cpp
)

# Export the include directory so others can use it
target_include_directories(Camera PUBLIC include)
target_link_libraries(Camera PUBLIC raylib)

target_link_libraries(Camera PUBLIC raylib)