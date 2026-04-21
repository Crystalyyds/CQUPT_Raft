include(FetchContent)
include(GoogleTest)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

add_executable(test_state_machine
  test_state_machine.cpp
)

target_link_libraries(test_state_machine
  PRIVATE
    raft_core
    GTest::gtest_main
)

gtest_discover_tests(test_state_machine)

add_executable(persistence_test
  persistence_test.cpp
)

target_link_libraries(persistence_test
  PRIVATE
    raft_core
)

add_test(
  NAME persistence_test
  COMMAND persistence_test
)