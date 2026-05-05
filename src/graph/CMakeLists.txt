add_library(ATGraph STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/impl/ATGraph.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/impl/GraphJson.cpp
)
target_include_directories(ATGraph PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(ATGraph PRIVATE "${CMAKE_SOURCE_DIR}/src")
# only target ATMath as math
target_link_libraries(ATGraph PUBLIC ATMath)
target_link_libraries(ATGraph PUBLIC nlohmann_json::nlohmann_json)