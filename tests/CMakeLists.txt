include(FetchContent)
include(GoogleTest)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

# Windows 下避免覆盖父工程的 runtime 配置
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