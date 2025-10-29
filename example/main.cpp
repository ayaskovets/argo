#include <sandbox/io/poller.hpp>
#include <sandbox/logging/logging.hpp>
#include <sandbox/net/dns.hpp>
#include <sandbox/net/inet/sockaddr.hpp>
#include <sandbox/net/inet/tcp/socket.hpp>
#include <sandbox/net/unix/dgram/socket.hpp>
#include <sandbox/net/unix/sockaddr.hpp>

#include <thread>

namespace {

namespace logging = sandbox::logging;
namespace net = sandbox::net;
namespace io = sandbox::io;

}  // namespace

int server() try {
  logging::debug("smoke: starting on {}:{}", "127.0.0.1", "7777");
  logging::info("smoke: starting on {}:{}", "127.0.0.1", "7777");
  logging::warning("smoke: starting on {}:{}", "127.0.0.1", "7777");
  logging::error("smoke: starting on {}:{}", "127.0.0.1", "7777");

  const auto tcp =
      net::resolve("127.0.0.1", net::family::kInet, net::protocol::kTcp);
  const auto udp =
      net::resolve("127.0.0.1", net::family::kInet, net::protocol::kUdp);

  logging::info("resolve: tcp[{}] udp[{}]", tcp.size(), udp.size());
  logging::info("resolve: tcp[1/{}] = {}", tcp.size(),
                std::get<net::inet::sockaddr>(tcp.at(0)));
  logging::info("resolve: udp[1/{}] = {}", udp.size(),
                std::get<net::inet::sockaddr>(udp.at(0)));

  const auto server_sockaddr =
      net::inet::sockaddr(net::inet::ip("127.0.0.1"), net::inet::port(8081));

  net::inet::tcp::socket server;
  server.set_flags(io::fd::flag::kNonblock);
  server.set_reuseaddr(true);
  server.set_reuseport(true);
  server.bind(server_sockaddr);
  server.listen(1);

  logging::info("server: accepting on {}...", server_sockaddr);

  net::inet::tcp::socket client;
  client.set_flags(io::fd::flag::kNonblock);
  client.set_reuseaddr(true);
  client.set_reuseport(true);

  if (client.connect(server_sockaddr) == net::connect_status::kPending) {
    logging::info("client: pending connection to {}", server_sockaddr);
  }

  std::this_thread::sleep_for(std::chrono::seconds(3));

  net::inet::tcp::socket peer(io::uninitialized_t{});
  if (server.accept(peer) == net::accept_status::kSuccess) {
    net::inet::sockaddr peer_sockaddr(net::inet::ip::kAny(),
                                      net::inet::port(0));

    peer.get_connect_sockaddr(peer_sockaddr);
    logging::info("server: got connection from {}", peer_sockaddr);
  } else {
    logging::warning("server: failed to get connection on {}", server_sockaddr);
  }

  if (client.connect(server_sockaddr) == net::connect_status::kSuccess) {
    logging::info("client: successful connection to {}", server_sockaddr);
  } else {
    logging::warning("client: still no success to {}", server_sockaddr);
  }
  return 0;
} catch (const std::exception& e) {
  puts(std::format("{}", e.what()).c_str());
  return 1;
}

int poll() try {
  io::poller poller([&poller](const io::fd& fd, io::poller::events_t events) {
    const auto& socket = reinterpret_cast<const net::inet::tcp::socket&>(fd);
    net::inet::sockaddr peer_sockaddr(net::inet::ip::kAny(),
                                      net::inet::port(0));
    socket.get_connect_sockaddr(peer_sockaddr);
    logging::info("poller: event from {}: {}", peer_sockaddr, events);

    if (events & io::poller::event::kPollHup) {
      poller.erase(fd);
    }
  });

  const auto server_sockaddr =
      net::inet::sockaddr(net::inet::ip("127.0.0.1"), net::inet::port(8081));

  net::inet::tcp::socket server;
  server.set_flags(io::fd::flag::kNonblock);
  server.set_reuseaddr(true);
  server.set_reuseport(true);
  server.bind(server_sockaddr);
  server.listen(1);

  logging::info("server: accepting on {}...", server_sockaddr);

  net::inet::tcp::socket client;
  client.set_flags(io::fd::flag::kNonblock);
  client.set_reuseaddr(true);
  client.set_reuseport(true);

  if (client.connect(server_sockaddr) == net::connect_status::kPending) {
    logging::info("client: pending connection to {}", server_sockaddr);
  }

  std::this_thread::sleep_for(std::chrono::seconds(3));

  net::inet::tcp::socket peer(io::uninitialized_t{});
  if (server.accept(peer) == net::accept_status::kSuccess) {
    net::inet::sockaddr peer_sockaddr(net::inet::ip::kAny(),
                                      net::inet::port(0));

    peer.get_connect_sockaddr(peer_sockaddr);
    logging::info("server: got connection from {}", peer_sockaddr);
  } else {
    logging::warning("server: failed to get connection on {}", server_sockaddr);
  }

  if (client.connect(server_sockaddr) == net::connect_status::kSuccess) {
    logging::info("client: successful connection to {}", server_sockaddr);
  } else {
    logging::warning("client: still no success to {}", server_sockaddr);
  }

  logging::info("poller: first poll");
  poller.poll_for(std::chrono::milliseconds{100});

  poller.insert_or_assign(
      peer, io::poller::event::kPollIn | io::poller::event::kPollOut);

  poller.insert_or_assign(
      client, io::poller::event::kPollIn | io::poller::event::kPollOut);

  std::vector<std::byte> bytes{std::byte(1), std::byte(2), std::byte(3)};
  peer.send(bytes);
  client.receive(bytes);

  logging::info("poller: second poll");
  poller.poll_for(std::chrono::milliseconds{100});

  peer.close();

  logging::info("poller: third poll");
  poller.poll_for(std::chrono::milliseconds{100});

  client.close();

  logging::info("poller: fourth poll");
  poller.poll_for(std::chrono::milliseconds{100});

  logging::info("poller: fifth poll");
  poller.poll_for(std::chrono::milliseconds{100});

  return 0;
} catch (const std::exception& e) {
  puts(std::format("{}", e.what()).c_str());
  return 1;
}

int main() { return poll(); }
