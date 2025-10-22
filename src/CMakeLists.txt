set(TARGET sandbox)

add_library(${TARGET} STATIC)
target_sources(${TARGET}
    PRIVATE
    datetime/iso8601.cpp
    io/base_fd.cpp
    io/poller.cpp
    logging/logger.cpp
    logging/renderer.cpp
    logging/writer.cpp
    net/base_sockaddr.cpp
    net/base_socket.cpp
    net/dns.cpp
    net/inet/ip.cpp
    net/inet/port.cpp
    net/inet/sockaddr.cpp
    net/inet/tcp/socket.cpp
    net/inet/udp/socket.cpp
    net/inet6/ip.cpp
    net/inet6/sockaddr.cpp
    net/inet6/tcp/socket.cpp
    net/inet6/udp/socket.cpp
    net/unix/base_socket.cpp
    net/unix/dgram/socket.cpp
    net/unix/sockaddr.cpp
    net/unix/stream/socket.cpp
)
target_compile_features(${TARGET}
    PRIVATE cxx_std_23
)
target_include_directories(${TARGET}
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)
