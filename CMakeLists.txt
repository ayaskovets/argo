cmake_minimum_required(VERSION 3.25.0)
project(
  argo
  VERSION 0.0.1
  LANGUAGES CXX)

# ------------------------------------------------------------------------------
# dependencies
# ------------------------------------------------------------------------------

find_package(swiss REQUIRED)
find_package(flow REQUIRED)

# ------------------------------------------------------------------------------
# general settings and options
# ------------------------------------------------------------------------------

option(BUILD_BENCHMARKS "Build the benchmarks tree." OFF)

add_library(argo)
add_library(argo::argo ALIAS argo)

# ------------------------------------------------------------------------------
# sources
# ------------------------------------------------------------------------------

include(GenerateExportHeader)

generate_export_header(argo EXPORT_FILE_NAME
                       "${CMAKE_CURRENT_BINARY_DIR}/include/argo/export.hpp")

set(SOURCES src/executors/this_executor.cpp)
set(HEADERS
    "${CMAKE_CURRENT_BINARY_DIR}/include/argo/export.hpp"
    include/argo/awaiters/continuation_awaiter.hpp
    include/argo/awaiters/proxy_awaiter.hpp
    include/argo/awaiters/timer_awaiter.hpp
    include/argo/awaiters/yield_awaiter.hpp
    include/argo/channel.hpp
    include/argo/execution_context.hpp
    include/argo/const_coroutine.hpp
    include/argo/executor.hpp
    include/argo/executors/inline_executor.hpp
    include/argo/executors/thread_pool_executor.hpp
    include/argo/scheduler.hpp
    include/argo/schedulers/timer_scheduler.hpp
    include/argo/this_executor.hpp)

# ------------------------------------------------------------------------------
# target
# ------------------------------------------------------------------------------

target_sources(
  argo
  PRIVATE ${SOURCES}
  PUBLIC FILE_SET
         HEADERS
         BASE_DIRS
         "${CMAKE_CURRENT_SOURCE_DIR}/include"
         "${CMAKE_CURRENT_BINARY_DIR}/include"
         FILES
         ${HEADERS})
target_compile_features(argo PUBLIC cxx_std_23)
set_target_properties(
  argo
  PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR}
             VERSION ${PROJECT_VERSION}
             POSITION_INDEPENDENT_CODE ON
             CXX_VISIBILITY_PRESET hidden
             VISIBILITY_INLINES_HIDDEN ON)

if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(argo PUBLIC argo_STATIC_DEFINE)
endif()

target_link_libraries(argo PUBLIC swiss::swiss)
target_link_libraries(argo PUBLIC flow::flow)

# ------------------------------------------------------------------------------
# install
# ------------------------------------------------------------------------------

if(PROJECT_IS_TOP_LEVEL)
  include(GNUInstallDirs)
  include(CMakePackageConfigHelpers)

  configure_package_config_file(
    cmake/argo-config.cmake.in argo-config.cmake
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/argo")

  write_basic_package_version_file(argo-config-version.cmake
                                   COMPATIBILITY SameMajorVersion)

  install(
    TARGETS argo
    EXPORT argo_export
    RUNTIME COMPONENT argo
    LIBRARY COMPONENT argo NAMELINK_COMPONENT argo-dev
    ARCHIVE COMPONENT argo-dev
            FILE_SET HEADERS
            COMPONENT argo-dev)

  install(
    EXPORT argo_export
    COMPONENT argo-dev
    FILE "argo-targets.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/argo"
    NAMESPACE argo::)

  install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/argo-config.cmake"
          "${CMAKE_CURRENT_BINARY_DIR}/argo-config-version.cmake"
    COMPONENT argo-dev
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/argo")
endif()

# ------------------------------------------------------------------------------
# benchmarks
# ------------------------------------------------------------------------------

if(PROJECT_IS_TOP_LEVEL)
  if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
  endif()
endif()

# ------------------------------------------------------------------------------
# tests
# ------------------------------------------------------------------------------
if(PROJECT_IS_TOP_LEVEL)
  include(CTest)

  if(BUILD_TESTING)
    add_subdirectory(tests/unit)
  endif()
endif()
