add_library(ATMath STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/ATMath.cpp
)
target_include_directories(ATMath PUBLIC include)
target_include_directories(ATMath PRIVATE "${CMAKE_SOURCE_DIR}/src")

target_link_libraries(ATMath PUBLIC raylib)
target_link_libraries(ATMath PUBLIC glm::glm)