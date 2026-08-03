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
    awaiters/continuation_awaiter_test.cpp
    awaiters/proxy_awaiter_test.cpp
    awaiters/suspend_always_awaiter_test.cpp
    awaiters/suspend_never_awaiter_test.cpp
    channel_test.cpp executor_test.cpp
    # scheduler_test.cpp
    task_test.cpp
    )

# ------------------------------------------------------------------------------
# target
# ------------------------------------------------------------------------------

target_sources(argo-unit-tests PRIVATE ${SOURCES})

target_link_libraries(argo-unit-tests PRIVATE argo::argo GTest::gtest_main)

gtest_discover_tests(argo-unit-tests)
