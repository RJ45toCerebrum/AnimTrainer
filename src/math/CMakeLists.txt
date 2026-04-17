add_library(ATMath STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/ATMath.cpp
)
target_include_directories(ATMath PUBLIC include)
target_link_libraries(ATMath PUBLIC raylib)
target_link_libraries(ATMath PRIVATE glm::glm)