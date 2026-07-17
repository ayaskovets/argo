cmake_minimum_required(VERSION 3.25.0)
project(argo-benchmarks LANGUAGES CXX)

# ------------------------------------------------------------------------------
# dependencies
# ------------------------------------------------------------------------------

find_package(benchmark REQUIRED)

if(PROJECT_IS_TOP_LEVEL)
  find_package(argo REQUIRED)
endif()

include(CTest)
include(GoogleTest)

# ------------------------------------------------------------------------------
# general settings and options
# ------------------------------------------------------------------------------

add_executable(argo-benchmarks)

# ------------------------------------------------------------------------------
# sources
# ------------------------------------------------------------------------------

set(SOURCES main.cpp)

# ------------------------------------------------------------------------------
# target
# ------------------------------------------------------------------------------

target_sources(argo-benchmarks PRIVATE ${SOURCES})

target_link_libraries(argo-benchmarks PRIVATE argo::argo benchmark::benchmark)

target_include_directories(argo-benchmarks PRIVATE ${CMAKE_CURRENT_LIST_DIR})

# ------------------------------------------------------------------------------
# workflow automation
# ------------------------------------------------------------------------------

add_test(NAME argo-benchmarks COMMAND argo-benchmarks --benchmark_color=true)
set_tests_properties(argo-benchmarks PROPERTIES LABELS benchmark)
