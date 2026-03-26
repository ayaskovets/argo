cmake_minimum_required(VERSION 3.23.0)
project(
  wip
  VERSION 0.0.1
  LANGUAGES CXX)

# ------------------------------------------------------------------------------
# dependencies
# ------------------------------------------------------------------------------

find_package(
  swiss REQUIRED PATHS
  "/Users/ayaskovets/Private/wip_personal/code/swiss/build/install/install")

# ------------------------------------------------------------------------------
# general settings and options
# ------------------------------------------------------------------------------

option(BUILD_BENCHMARKS "Build the benchmarks tree." OFF)

add_library(wip)
add_library(wip::wip ALIAS wip)

# ------------------------------------------------------------------------------
# sources
# ------------------------------------------------------------------------------

include(GenerateExportHeader)

generate_export_header(wip EXPORT_FILE_NAME
                       "${CMAKE_CURRENT_BINARY_DIR}/include/wip/export.hpp")

set(SOURCES
    src/async/executors/current_executor.cpp
    src/datetime/iso8601.cpp
    src/io/fd.cpp
    src/io/file.cpp
    src/io/poller.cpp
    src/logging/logger.cpp
    src/logging/renderer.cpp
    src/logging/writer.cpp
    src/net/base_sockaddr.cpp
    src/net/base_socket.cpp
    src/net/dns.cpp
    src/net/inet/ip.cpp
    src/net/inet/port.cpp
    src/net/inet/sockaddr.cpp
    src/net/inet/tcp/socket.cpp
    src/net/inet/udp/socket.cpp
    src/net/inet6/ip.cpp
    src/net/inet6/sockaddr.cpp
    src/net/inet6/tcp/socket.cpp
    src/net/inet6/udp/socket.cpp
    src/net/unix/base_socket.cpp
    src/net/unix/dgram/socket.cpp
    src/net/unix/sockaddr.cpp
    src/net/unix/stream/socket.cpp)
set(HEADERS
    "${CMAKE_CURRENT_BINARY_DIR}/include/wip/export.hpp"
    include/wip/net/protocol.hpp
    include/wip/net/base_socket.hpp
    include/wip/net/dns.hpp
    include/wip/net/base_sockaddr.hpp
    include/wip/net/family.hpp
    include/wip/net/network_byte_order.hpp
    include/wip/net/socktype.hpp
    include/wip/net/unix/base_socket.hpp
    include/wip/net/unix/stream/socket.hpp
    include/wip/net/unix/dgram/socket.hpp
    include/wip/net/unix/sockaddr.hpp
    include/wip/net/inet/port.hpp
    include/wip/net/inet/udp/socket.hpp
    include/wip/net/inet/tcp/socket.hpp
    include/wip/net/inet/ip.hpp
    include/wip/net/inet/sockaddr.hpp
    include/wip/net/inet6/port.hpp
    include/wip/net/inet6/udp/socket.hpp
    include/wip/net/inet6/tcp/socket.hpp
    include/wip/net/inet6/ip.hpp
    include/wip/net/inet6/sockaddr.hpp
    include/wip/datetime/iso8601.hpp
    include/wip/async/task.hpp
    include/wip/async/channel.hpp
    include/wip/async/executors/current_executor.hpp
    include/wip/async/executors/inline_executor.hpp
    include/wip/async/executors/thread_pool_executor.hpp
    include/wip/async/awaiters/io_awaiter.hpp
    include/wip/async/awaiters/timer_awaiter.hpp
    include/wip/async/scheduler.hpp
    include/wip/async/executor.hpp
    include/wip/async/schedulers/conditional_decorator.hpp
    include/wip/async/schedulers/yield_scheduler.hpp
    include/wip/async/schedulers/combinator.hpp
    include/wip/async/schedulers/io_scheduler.hpp
    include/wip/async/schedulers/detached_decorator.hpp
    include/wip/async/schedulers/timer_scheduler.hpp
    include/wip/async/schedulers/timed_decorator.hpp
    include/wip/async/channels/multiplexed_channel.hpp
    include/wip/io/fd_impl.hpp
    include/wip/io/file.hpp
    include/wip/io/fd.hpp
    include/wip/io/uninitialized.hpp
    include/wip/io/poller.hpp
    include/wip/utils/syscall.hpp
    include/wip/queues/lockfree_mpsc_queue.hpp
    include/wip/queues/locked_std_deque.hpp
    include/wip/queues/lockfree_mpmc_queue.hpp
    include/wip/queues/lockfree_spmc_queue.hpp
    include/wip/queues/locked_mpmc_queue.hpp
    include/wip/queues/waitfree_spsc_queue.hpp
    include/wip/queues/queue.hpp
    include/wip/queues/lockless_mpmc_queue.hpp
    include/wip/queues/single_threaded_queue.hpp
    include/wip/logging/logger.hpp
    include/wip/logging/writer.hpp
    include/wip/logging/renderer.hpp
    include/wip/logging/level.hpp
    include/wip/logging/logging.hpp)

# ------------------------------------------------------------------------------
# target
# ------------------------------------------------------------------------------

target_sources(
  wip
  PRIVATE ${SOURCES}
  PUBLIC FILE_SET
         HEADERS
         BASE_DIRS
         "${CMAKE_CURRENT_SOURCE_DIR}/include"
         "${CMAKE_CURRENT_BINARY_DIR}/include"
         FILES
         ${HEADERS})
target_compile_features(wip PUBLIC cxx_std_23)
set_target_properties(
  wip
  PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR}
             VERSION ${PROJECT_VERSION}
             POSITION_INDEPENDENT_CODE ON
             CXX_VISIBILITY_PRESET hidden
             VISIBILITY_INLINES_HIDDEN ON)

if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(wip PUBLIC WIP_STATIC_DEFINE)
endif()

target_link_libraries(wip PUBLIC swiss::swiss)

# ------------------------------------------------------------------------------
# install
# ------------------------------------------------------------------------------

if(PROJECT_IS_TOP_LEVEL)
  include(GNUInstallDirs)
  include(CMakePackageConfigHelpers)

  configure_package_config_file(
    cmake/wip-config.cmake.in wip-config.cmake
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/wip")

  write_basic_package_version_file(wip-config-version.cmake
                                   COMPATIBILITY SameMajorVersion)

  install(
    TARGETS wip
    EXPORT wip_export
    RUNTIME COMPONENT wip
    LIBRARY COMPONENT wip NAMELINK_COMPONENT wip-dev
    ARCHIVE COMPONENT wip-dev
            FILE_SET HEADERS
            COMPONENT wip-dev)

  install(
    EXPORT wip_export
    COMPONENT wip-dev
    FILE "wip-targets.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/wip"
    NAMESPACE wip::)

  install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/wip-config.cmake"
          "${CMAKE_CURRENT_BINARY_DIR}/wip-config-version.cmake"
    COMPONENT wip-dev
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/wip")
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
