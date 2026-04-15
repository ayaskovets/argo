cmake_minimum_required(VERSION 3.25.0)
project(wip-unit-tests LANGUAGES CXX)

# ------------------------------------------------------------------------------
# dependencies
# ------------------------------------------------------------------------------

find_package(GTest REQUIRED)

if(PROJECT_IS_TOP_LEVEL)
  find_package(wip REQUIRED)
endif()

include(CTest)
include(GoogleTest)

# ------------------------------------------------------------------------------
# general settings and options
# ------------------------------------------------------------------------------

add_executable(wip-unit-tests)

# ------------------------------------------------------------------------------
# sources
# ------------------------------------------------------------------------------

set(SOURCES
    io/fd_test.cpp
    io/file_test.cpp
    io/poller_test.cpp
    logging/logger_test.cpp
    net/base_sockaddr_test.cpp
    net/base_socket_test.cpp
    net/inet/ip_test.cpp
    net/inet/port_test.cpp
    net/inet/sockaddr_test.cpp
    net/inet/tcp/socket_test.cpp
    net/inet/udp/socket_test.cpp
    net/inet6/ip_test.cpp
    net/inet6/port_test.cpp
    net/inet6/sockaddr_test.cpp
    net/inet6/tcp/socket_test.cpp
    net/inet6/udp/socket_test.cpp
    net/resolve_test.cpp
    net/unix/dgram/socket_test.cpp
    net/unix/sockaddr_test.cpp
    net/unix/stream/socket_test.cpp)

# ------------------------------------------------------------------------------
# target
# ------------------------------------------------------------------------------

target_sources(wip-unit-tests PRIVATE ${SOURCES})

target_link_libraries(wip-unit-tests PRIVATE wip::wip GTest::gtest_main)

gtest_discover_tests(wip-unit-tests)
