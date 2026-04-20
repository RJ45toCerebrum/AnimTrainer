add_library(ATRandom STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/ATRandom.cpp
)
target_link_libraries(ATRandom PUBLIC glm::glm)