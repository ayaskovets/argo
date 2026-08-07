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

add_library(argo INTERFACE)
add_library(argo::argo ALIAS argo)

# ------------------------------------------------------------------------------
# sources
# ------------------------------------------------------------------------------

set(HEADERS
    include/argo/awaiters/continuation_awaiter.hpp
    include/argo/awaiters/proxy_awaiter.hpp
    include/argo/awaiters/suspend_always_awaiter.hpp
    include/argo/awaiters/suspend_never_awaiter.hpp
    include/argo/awaiters/yield_awaiter.hpp
    include/argo/channel.hpp
    include/argo/execution_context.hpp
    include/argo/executor.hpp
    include/argo/executors/inline_executor.hpp
    include/argo/executors/thread_pool_executor.hpp
    include/argo/scheduler.hpp
    include/argo/task_ref.hpp
    include/argo/task.hpp)

# ------------------------------------------------------------------------------
# target
# ------------------------------------------------------------------------------

target_sources(
  argo INTERFACE FILE_SET HEADERS BASE_DIRS
                 "${CMAKE_CURRENT_SOURCE_DIR}/include" FILES ${HEADERS})

target_compile_features(argo INTERFACE cxx_std_23)

target_link_libraries(argo INTERFACE swiss::swiss)
target_link_libraries(argo INTERFACE flow::flow)

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
    FILE_SET HEADERS)

  install(
    EXPORT argo_export
    FILE "argo-targets.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/argo"
    NAMESPACE argo::)

  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/argo-config.cmake"
                "${CMAKE_CURRENT_BINARY_DIR}/argo-config-version.cmake"
          DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/argo")
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
