add_executable(AnimTrainerTests
        GraphTests.cpp
)
target_link_libraries(AnimTrainerTests PRIVATE
        GTest::gtest_main
        ATMath
        ATGraph
)
target_include_directories(AnimTrainerTests PRIVATE
        "${CMAKE_SOURCE_DIR}/src"
)
include(GoogleTest)
gtest_discover_tests(AnimTrainerTests)