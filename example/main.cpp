#include <sandbox/io/poller.hpp>
#include <sandbox/logging/logging.hpp>
#include <sandbox/net/dns.hpp>
#include <sandbox/net/inet/sockaddr.hpp>
#include <sandbox/net/inet/tcp/socket.hpp>
#include <sandbox/net/unix/dgram/socket.hpp>
#include <sandbox/net/unix/sockaddr.hpp>

#include <latch>
#include <thread>

int server() try {
  sandbox::logging::debug("smoke: starting on {}:{}", "127.0.0.1", "7777");
  sandbox::logging::info("smoke: starting on {}:{}", "127.0.0.1", "7777");
  sandbox::logging::warning("smoke: starting on {}:{}", "127.0.0.1", "7777");
  sandbox::logging::error("smoke: starting on {}:{}", "127.0.0.1", "7777");

  const auto tcp = sandbox::net::resolve(
      "127.0.0.1", sandbox::net::family::kInet, sandbox::net::protocol::kTcp);
  const auto udp = sandbox::net::resolve(
      "127.0.0.1", sandbox::net::family::kInet, sandbox::net::protocol::kUdp);

  sandbox::logging::info("resolve: tcp[{}] udp[{}]", tcp.size(), udp.size());
  sandbox::logging::info("resolve: tcp[1/{}] = {}", tcp.size(),
                         std::get<sandbox::net::inet::sockaddr>(tcp.at(0)));
  sandbox::logging::info("resolve: udp[1/{}] = {}", udp.size(),
                         std::get<sandbox::net::inet::sockaddr>(udp.at(0)));

  const auto server_sockaddr = sandbox::net::inet::sockaddr(
      sandbox::net::inet::ip("127.0.0.1"), sandbox::net::inet::port(8081));

  sandbox::net::inet::tcp::socket server;
  server.set_nonblock(true);
  server.set_reuseaddr(true);
  server.set_reuseport(true);
  server.bind(server_sockaddr);
  server.listen(1);

  sandbox::logging::info("server: accepting on {}...", server_sockaddr);

  sandbox::net::inet::tcp::socket client;
  client.set_nonblock(true);
  client.set_reuseaddr(true);
  client.set_reuseport(true);

  if (client.connect(server_sockaddr) ==
      sandbox::net::connect_status::kPending) {
    sandbox::logging::info("client: pending connection to {}", server_sockaddr);
  }

  std::this_thread::sleep_for(std::chrono::seconds(3));

  sandbox::net::inet::tcp::socket peer(sandbox::io::uninitialized_t{});
  if (server.accept(peer) == sandbox::net::accept_status::kSuccess) {
    sandbox::net::inet::sockaddr peer_sockaddr(sandbox::net::inet::ip::kAny(),
                                               sandbox::net::inet::port(0));

    peer.get_connect_sockaddr(peer_sockaddr);
    sandbox::logging::info("server: got connection from {}", peer_sockaddr);
  } else {
    sandbox::logging::warning("server: failed to get connection on {}",
                              server_sockaddr);
  }

  if (client.connect(server_sockaddr) ==
      sandbox::net::connect_status::kSuccess) {
    sandbox::logging::info("client: successful connection to {}",
                           server_sockaddr);
  } else {
    sandbox::logging::warning("client: still no success to {}",
                              server_sockaddr);
  }
  return 0;
} catch (const std::exception& e) {
  puts(std::format("{}", e.what()).c_str());
  return 1;
}

int poll() try {
  sandbox::io::poller poller([&poller](const sandbox::io::base_fd& base_fd,
                                       sandbox::io::poller::events_t events) {
    const auto& socket =
        reinterpret_cast<const sandbox::net::inet::tcp::socket&>(base_fd);
    sandbox::net::inet::sockaddr peer_sockaddr(sandbox::net::inet::ip::kAny(),
                                               sandbox::net::inet::port(0));
    socket.get_connect_sockaddr(peer_sockaddr);
    sandbox::logging::info("poller: event from {}: {}", peer_sockaddr, events);

    if (events & sandbox::io::poller::event::kPollHup) {
      poller.erase(base_fd);
    }
  });

  const auto server_sockaddr = sandbox::net::inet::sockaddr(
      sandbox::net::inet::ip("127.0.0.1"), sandbox::net::inet::port(8081));

  sandbox::net::inet::tcp::socket server;
  server.set_nonblock(true);
  server.set_reuseaddr(true);
  server.set_reuseport(true);
  server.bind(server_sockaddr);
  server.listen(1);

  sandbox::logging::info("server: accepting on {}...", server_sockaddr);

  sandbox::net::inet::tcp::socket client;
  client.set_nonblock(true);
  client.set_reuseaddr(true);
  client.set_reuseport(true);

  if (client.connect(server_sockaddr) ==
      sandbox::net::connect_status::kPending) {
    sandbox::logging::info("client: pending connection to {}", server_sockaddr);
  }

  std::this_thread::sleep_for(std::chrono::seconds(3));

  sandbox::net::inet::tcp::socket peer(sandbox::io::uninitialized_t{});
  if (server.accept(peer) == sandbox::net::accept_status::kSuccess) {
    sandbox::net::inet::sockaddr peer_sockaddr(sandbox::net::inet::ip::kAny(),
                                               sandbox::net::inet::port(0));

    peer.get_connect_sockaddr(peer_sockaddr);
    sandbox::logging::info("server: got connection from {}", peer_sockaddr);
  } else {
    sandbox::logging::warning("server: failed to get connection on {}",
                              server_sockaddr);
  }

  if (client.connect(server_sockaddr) ==
      sandbox::net::connect_status::kSuccess) {
    sandbox::logging::info("client: successful connection to {}",
                           server_sockaddr);
  } else {
    sandbox::logging::warning("client: still no success to {}",
                              server_sockaddr);
  }

  sandbox::logging::info("poller: first poll");
  poller.poll_for(std::chrono::milliseconds{100});

  poller.insert_or_assign(peer, sandbox::io::poller::event::kPollIn |
                                    sandbox::io::poller::event::kPollOut);

  poller.insert_or_assign(client, sandbox::io::poller::event::kPollIn |
                                      sandbox::io::poller::event::kPollOut);

  std::vector<std::byte> bytes{std::byte(1), std::byte(2), std::byte(3)};
  peer.send(bytes);
  client.receive(bytes);

  sandbox::logging::info("poller: second poll");
  poller.poll_for(std::chrono::milliseconds{100});

  peer.close();

  sandbox::logging::info("poller: third poll");
  poller.poll_for(std::chrono::milliseconds{100});

  client.close();

  sandbox::logging::info("poller: fourth poll");
  poller.poll_for(std::chrono::milliseconds{100});

  sandbox::logging::info("poller: fifth poll");
  poller.poll_for(std::chrono::milliseconds{100});

  return 0;
} catch (const std::exception& e) {
  puts(std::format("{}", e.what()).c_str());
  return 1;
}

int main() { return poll(); }
