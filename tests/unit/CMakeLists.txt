cmake_minimum_required(VERSION 3.25.0)
project(argo-unit-tests LANGUAGES CXX)

# ------------------------------------------------------------------------------
# dependencies
# ------------------------------------------------------------------------------

find_package(GTest REQUIRED)

if(PROJECT_IS_TOP_LEVEL)
  find_package(argo REQUIRED)
endif()

include(CTest)
include(GoogleTest)

# ------------------------------------------------------------------------------
# general settings and options
# ------------------------------------------------------------------------------

add_executable(argo-unit-tests)

# ------------------------------------------------------------------------------
# sources
# ------------------------------------------------------------------------------

set(SOURCES
    awaiter_test.cpp
    awaiters/async_awaiter_test.cpp
    awaiters/detach_awaiter_test.cpp
    awaiters/timer_awaiter_test.cpp
    awaiters/yield_awaiter_test.cpp
    channel_test.cpp
    channels/multiplexed_channel_test.cpp
    execution_context_test.cpp
    executor_test.cpp
    executors/inline_executor_test.cpp
    executors/thread_pool_executor_test.cpp
    promise_ptr_test.cpp
    scheduler_test.cpp
    # schedulers/io_scheduler_test.cpp
    schedulers/timer_scheduler_test.cpp
    task_future_test.cpp
    task_test.cpp)

# ------------------------------------------------------------------------------
# target
# ------------------------------------------------------------------------------

target_sources(argo-unit-tests PRIVATE ${SOURCES})

target_link_libraries(argo-unit-tests PRIVATE argo::argo GTest::gtest_main)

gtest_discover_tests(argo-unit-tests)
