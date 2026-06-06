include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

file(GLOB_RECURSE TEST_SOURCES "*.cpp")

add_executable(draughts_test ${TEST_SOURCES})
target_link_libraries(draughts_test PRIVATE draughts_core GTest::gtest_main)
set_property(TARGET draughts_test PROPERTY CXX_STANDARD 20)

include(GoogleTest)
gtest_discover_tests(draughts_test)
