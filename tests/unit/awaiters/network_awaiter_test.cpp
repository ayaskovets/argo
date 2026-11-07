#include <argo/awaiters/network_awaiter.hpp>
#include <argo/awaiters/yield_awaiter.hpp>
#include <argo/executors/inline_executor.hpp>
#include <argo/task.hpp>

#include <flow/spsc_queue.hpp>
#include <gtest/gtest.h>
#include <skynet/inet/sockaddr.hpp>
#include <skynet/inet/tcp/socket.hpp>
#include <skynet/inet/udp/socket.hpp>
#include <skynet/options/options.hpp>

namespace tests::unit::awaiters {

TEST(network_awaiter, accept) {
  skynet::inet::tcp::socket acceptor;
  skynet::inet::tcp::socket client;
  skynet::inet::tcp::socket peer(skynet::uninitialized);
  skynet::inet::sockaddr address(skynet::inet::ip::kLoopback(),
                                 skynet::inet::port(0));
  skynet::reactor reactor(1);

  flow::spsc_queue<argo::execution_token> in_channel(1);
  flow::spsc_queue<argo::schedulers::network_in_value *> out_channel(1);
  argo::executors::inline_executor executor(in_channel, out_channel);
  argo::schedulers::network_in_value * in_value = nullptr;

  ASSERT_EQ(acceptor.bind(address), skynet::bind_status::kSuccess);
  acceptor.getsockname(address);
  acceptor.listen(1);
  acceptor.set_option(skynet::options::nonblock(true));
  reactor.insert_or_assign(acceptor, skynet::reactor::kPollIn, nullptr);

  auto task = [](skynet::inet::tcp::socket * acceptor,
                 skynet::inet::tcp::socket * peer)
      -> argo::task<skynet::accept_status> {
    co_return co_await argo::awaiters::accept(*acceptor, *peer);
  }(&acceptor, &peer);

  in_channel.push(task.get_execution_token());
  executor.run_once();
  EXPECT_FALSE(task.done());
  EXPECT_EQ(out_channel.size(), 1);
  ASSERT_EQ(client.connect(address), skynet::connect_status::kSuccess);
  ASSERT_FALSE(reactor.poll(std::chrono::seconds(1)).empty());
  ASSERT_TRUE(out_channel.try_pop(in_value));

  in_channel.push(in_value->get_execution_token());
  executor.run_once();
  ASSERT_TRUE(task.done());
  EXPECT_EQ(task.get(), skynet::accept_status::kSuccess);

  peer.close();
  task = [](skynet::inet::tcp::socket * acceptor,
            skynet::inet::tcp::socket * peer)
      -> argo::task<skynet::accept_status> {
    co_return co_await argo::awaiters::accept(*acceptor, *peer);
  }(&acceptor, &peer);

  in_channel.push(task.get_execution_token());
  executor.run_once();
  EXPECT_FALSE(task.done());
  ASSERT_TRUE(out_channel.try_pop(in_value));

  in_channel.push(in_value->get_execution_token());
  executor.run_once();
  ASSERT_TRUE(task.done());
  EXPECT_EQ(task.get(), skynet::accept_status::kEmptyQueue);
}

TEST(network_awaiter, connect) {
  skynet::inet::udp::socket socket;
  skynet::inet::sockaddr address(skynet::inet::ip::kLoopback(),
                                 skynet::inet::port(0));
  skynet::inet::sockaddr peer_address(address);

  ASSERT_EQ(socket.bind(address), skynet::bind_status::kSuccess);
  socket.getsockname(address);
  socket.set_option(skynet::options::nonblock(true));

  auto task = [](skynet::inet::udp::socket * socket,
                 skynet::inet::sockaddr * address)
      -> argo::task<skynet::connect_status> {
    co_return co_await argo::awaiters::connect(*socket, *address);
  }(&socket, &address);

  EXPECT_THROW(socket.getpeername(peer_address), std::runtime_error);

  task.resume();
  ASSERT_TRUE(task.done());
  EXPECT_EQ(task.get(), skynet::connect_status::kSuccess);
  socket.getpeername(peer_address);
  EXPECT_EQ(peer_address, address);
}

TEST(network_awaiter, send_recv) {
  skynet::inet::udp::socket sender;
  skynet::inet::udp::socket receiver;
  skynet::inet::sockaddr address(skynet::inet::ip::kLoopback(),
                                 skynet::inet::port(0));
  skynet::reactor reactor(1);
  std::array const bytes{std::byte{1}, std::byte{2}};
  std::array<std::byte, 4> received{};

  ASSERT_EQ(receiver.bind(address), skynet::bind_status::kSuccess);
  receiver.getsockname(address);
  ASSERT_EQ(sender.connect(address), skynet::connect_status::kSuccess);
  sender.set_option(skynet::options::nonblock(true));
  receiver.set_option(skynet::options::nonblock(true));
  reactor.insert_or_assign(receiver, skynet::reactor::kPollIn, nullptr);

  auto send = [](skynet::inet::udp::socket * socket,
                 std::span<std::byte const> bytes) -> argo::task<std::size_t> {
    co_return co_await argo::awaiters::send(*socket, bytes);
  }(&sender, bytes);
  auto recv = [](skynet::inet::udp::socket * socket,
                 std::span<std::byte> bytes) -> argo::task<std::size_t> {
    co_return co_await argo::awaiters::recv(*socket, bytes);
  }(&receiver, received);

  send.resume();
  ASSERT_TRUE(send.done());
  EXPECT_EQ(send.get(), bytes.size());
  ASSERT_FALSE(reactor.poll(std::chrono::seconds(1)).empty());

  recv.resume();
  ASSERT_TRUE(recv.done());
  EXPECT_EQ(recv.get(), bytes.size());
  EXPECT_EQ(received.front(), bytes.front());
  EXPECT_EQ(received[1], bytes.back());
  EXPECT_EQ(receiver.recv(received), skynet::base_socket::kWouldBlock);
}

TEST(network_awaiter, sendto_recvfrom) {
  skynet::inet::udp::socket sender;
  skynet::inet::udp::socket receiver;
  skynet::inet::sockaddr address(skynet::inet::ip::kLoopback(),
                                 skynet::inet::port(0));
  skynet::inet::sockaddr peer_address(address);
  skynet::reactor reactor(1);
  std::array const bytes{std::byte{1}, std::byte{2}};
  std::array<std::byte, 2> received{};

  flow::spsc_queue<argo::execution_token> in_channel(1);
  flow::spsc_queue<argo::schedulers::network_in_value *> out_channel(1);
  argo::executors::inline_executor executor(in_channel, out_channel);
  argo::schedulers::network_in_value * in_value = nullptr;

  ASSERT_EQ(sender.bind(address), skynet::bind_status::kSuccess);
  ASSERT_EQ(receiver.bind(address), skynet::bind_status::kSuccess);
  receiver.getsockname(address);
  sender.set_option(skynet::options::nonblock(true));
  receiver.set_option(skynet::options::nonblock(true));
  reactor.insert_or_assign(receiver, skynet::reactor::kPollIn, nullptr);

  auto recv = [](skynet::inet::udp::socket * socket, std::span<std::byte> bytes,
                 skynet::inet::sockaddr * address) -> argo::task<std::size_t> {
    co_return co_await argo::awaiters::recvfrom(*socket, bytes, *address);
  }(&receiver, received, &peer_address);
  auto send =
      [](skynet::inet::udp::socket * socket, std::span<std::byte const> bytes,
         skynet::inet::sockaddr const * address) -> argo::task<std::size_t> {
    co_return co_await argo::awaiters::sendto(*socket, bytes, *address);
  }(&sender, bytes, &address);

  in_channel.push(recv.get_execution_token());
  executor.run_once();
  EXPECT_FALSE(recv.done());
  EXPECT_EQ(out_channel.size(), 1);
  send.resume();
  ASSERT_TRUE(send.done());
  EXPECT_EQ(send.get(), bytes.size());
  ASSERT_FALSE(reactor.poll(std::chrono::seconds(1)).empty());
  ASSERT_TRUE(out_channel.try_pop(in_value));

  in_channel.push(in_value->get_execution_token());
  executor.run_once();
  ASSERT_TRUE(recv.done());
  EXPECT_EQ(recv.get(), bytes.size());
  EXPECT_EQ(received, bytes);
  sender.getsockname(address);
  EXPECT_EQ(peer_address, address);
}

TEST(network_awaiter, would_block) {
  skynet::inet::udp::socket socket;
  std::array<std::byte, 1> bytes{};

  flow::spsc_queue<argo::execution_token> in_channel(1);
  flow::spsc_queue<argo::schedulers::network_in_value *> out_channel(1);
  argo::executors::inline_executor executor(in_channel, out_channel);
  argo::schedulers::network_in_value * in_value = nullptr;

  socket.set_option(skynet::options::nonblock(true));

  auto task = [](skynet::inet::udp::socket * socket,
                 std::span<std::byte> bytes) -> argo::task<std::size_t> {
    co_return co_await argo::awaiters::recv(*socket, bytes);
  }(&socket, bytes);

  in_channel.push(task.get_execution_token());
  executor.run_once();
  EXPECT_FALSE(task.done());
  EXPECT_TRUE(in_channel.empty());
  ASSERT_TRUE(out_channel.try_pop(in_value));
  EXPECT_EQ(&in_value->get_socket(), &socket);
  EXPECT_EQ(in_value->get_filter(), skynet::reactor::kPollIn);

  in_channel.push(in_value->get_execution_token());
  executor.run_once();
  EXPECT_TRUE(task.done());
  EXPECT_EQ(task.get(), skynet::base_socket::kWouldBlock);
  EXPECT_TRUE(out_channel.empty());
}

TEST(network_awaiter, partial_send) {
  constexpr std::uint32_t kSendBufferSize    = 4096;
  constexpr std::uint32_t kReceiveBufferSize = 65536;
  constexpr std::size_t kDataSize            = 1'048'576;

  skynet::inet::tcp::socket acceptor;
  skynet::inet::tcp::socket sender;
  skynet::inet::tcp::socket receiver(skynet::uninitialized);
  skynet::inet::sockaddr address(skynet::inet::ip::kLoopback(),
                                 skynet::inet::port(0));
  std::vector<std::byte> bytes(kDataSize);

  ASSERT_EQ(acceptor.bind(address), skynet::bind_status::kSuccess);
  acceptor.getsockname(address);
  acceptor.listen(1);
  ASSERT_EQ(sender.connect(address), skynet::connect_status::kSuccess);
  ASSERT_EQ(acceptor.accept(receiver), skynet::accept_status::kSuccess);
  sender.set_option(skynet::options::nonblock(true));
  sender.set_option(skynet::options::sndbuf(kSendBufferSize));
  receiver.set_option(skynet::options::rcvbuf(kReceiveBufferSize));

  auto task = [](skynet::inet::tcp::socket * socket,
                 std::span<std::byte const> bytes) -> argo::task<std::size_t> {
    co_return co_await argo::awaiters::send(*socket, bytes);
  }(&sender, bytes);

  task.resume();
  ASSERT_TRUE(task.done());
  auto const size = task.get();
  EXPECT_GT(size, 0);
  EXPECT_LT(size, bytes.size());
}

TEST(network_awaiter, eof) {
  skynet::inet::tcp::socket acceptor;
  skynet::inet::tcp::socket receiver;
  skynet::inet::tcp::socket sender(skynet::uninitialized);
  skynet::inet::sockaddr address(skynet::inet::ip::kLoopback(),
                                 skynet::inet::port(0));
  skynet::reactor reactor(1);
  std::array<std::byte, 1> bytes{};

  flow::spsc_queue<argo::execution_token> in_channel(1);
  flow::spsc_queue<argo::schedulers::network_in_value *> out_channel(1);
  argo::executors::inline_executor executor(in_channel, out_channel);
  argo::schedulers::network_in_value * in_value = nullptr;

  ASSERT_EQ(acceptor.bind(address), skynet::bind_status::kSuccess);
  acceptor.getsockname(address);
  acceptor.listen(1);
  ASSERT_EQ(receiver.connect(address), skynet::connect_status::kSuccess);
  ASSERT_EQ(acceptor.accept(sender), skynet::accept_status::kSuccess);
  receiver.set_option(skynet::options::nonblock(true));
  reactor.insert_or_assign(receiver, skynet::reactor::kPollIn, nullptr);

  auto task = [](skynet::inet::tcp::socket * socket,
                 std::span<std::byte> bytes) -> argo::task<std::size_t> {
    co_return co_await argo::awaiters::recv(*socket, bytes);
  }(&receiver, bytes);

  in_channel.push(task.get_execution_token());
  executor.run_once();
  EXPECT_FALSE(task.done());
  EXPECT_EQ(out_channel.size(), 1);
  sender.close();
  ASSERT_FALSE(reactor.poll(std::chrono::seconds(1)).empty());
  ASSERT_TRUE(out_channel.try_pop(in_value));

  in_channel.push(in_value->get_execution_token());
  executor.run_once();
  ASSERT_TRUE(task.done());
  EXPECT_EQ(task.get(), 0);
}

TEST(network_awaiter, exception) {
  skynet::inet::tcp::socket socket;
  skynet::inet::sockaddr const address(skynet::inet::ip::kLoopback(),
                                       skynet::inet::port(0));

  socket.close();

  auto task = [](skynet::inet::tcp::socket * socket,
                 skynet::inet::sockaddr const * address) -> argo::task<void> {
    co_await argo::awaiters::connect(*socket, *address);
  }(&socket, &address);

  task.resume();
  EXPECT_TRUE(task.done());
  EXPECT_THROW(task.get(), std::runtime_error);
}

TEST(network_awaiter, incompatible_out_channel_type) {
  skynet::inet::udp::socket socket;
  flow::spsc_queue<argo::execution_token> in_channel(1);
  flow::spsc_queue<std::nullptr_t> out_channel(1);
  argo::executors::inline_executor executor(in_channel, out_channel);

  class operation final {
   public:
    constexpr auto await_ready() const noexcept -> bool { return false; }
    constexpr void await_resume(skynet::reactor::event_filter) const noexcept {}
  };

  bool caught = false;
  auto task   = [](skynet::inet::udp::socket * socket,
                 bool * caught) -> argo::task<void> {
    try {
      co_await argo::awaiters::network(*socket, skynet::reactor::kPollIn,
                                         operation());
    } catch (std::runtime_error const &) {
      *caught = true;
    }

    co_await argo::awaiters::yield();
  }(&socket, &caught);

  in_channel.push(task.get_execution_token());
  executor.run_once();
  EXPECT_TRUE(caught);
  EXPECT_FALSE(task.done());

  executor.run_once();
  EXPECT_TRUE(task.done());
  EXPECT_NO_THROW(task.get());
}

}  // namespace tests::unit::awaiters
