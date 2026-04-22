add_library(ATScene STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/ATScene.cpp
)
target_include_directories(ATScene PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(ATScene PRIVATE "${CMAKE_SOURCE_DIR}/src")
# only target ATMath as math
target_link_libraries(ATScene PUBLIC ATMath)