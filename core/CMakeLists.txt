set(TARGET core)

add_library(${TARGET} STATIC)
target_sources(${TARGET}
    PRIVATE
    src/datetime/iso8601.cpp
    src/io/fd.cpp
    src/io/poller.cpp
    src/logging/logger.cpp
    src/logging/renderer.cpp
    src/logging/writer.cpp
    src/net/base_sockaddr.cpp
    src/net/base_socket.cpp
    src/net/dns/resolve.cpp
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
    src/net/unix/stream/socket.cpp
)
target_compile_features(${TARGET}
    PRIVATE cxx_std_23
)
target_include_directories(${TARGET}
    PUBLIC include
)

add_subdirectory(benchmarks)
add_subdirectory(tests)
