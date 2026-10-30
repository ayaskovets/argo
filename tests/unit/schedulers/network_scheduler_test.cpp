#include <argo/awaiters/network_awaiter.hpp>
#include <argo/executors/inline_executor.hpp>
#include <argo/schedulers/network_scheduler.hpp>
#include <argo/task.hpp>

#include <flow/spsc_queue.hpp>
#include <gtest/gtest.h>
#include <skynet/inet/sockaddr.hpp>
#include <skynet/inet/tcp/socket.hpp>
#include <skynet/inet/udp/socket.hpp>
#include <skynet/options/options.hpp>

namespace tests::unit::schedulers {

TEST(network_scheduler, smoke) {
  skynet::inet::udp::socket socket;
  skynet::inet::sockaddr const address(skynet::inet::ip::kLoopback(),
                                       skynet::inet::port(0));

  flow::spsc_queue<argo::execution_token> in_channel(1);
  flow::spsc_queue<argo::schedulers::network_in_value *> out_channel(1);
  argo::executors::inline_executor executor(in_channel, out_channel);
  argo::schedulers::network_scheduler scheduler(out_channel, in_channel);

  ASSERT_EQ(socket.bind(address), skynet::bind_status::kSuccess);

  auto task = []() -> argo::task<void> { co_return; }();
  argo::schedulers::network_in_value in_value(
      task.get_execution_token(), socket, skynet::reactor::kPollOut);

  scheduler.run_once();
  EXPECT_TRUE(in_channel.empty());

  out_channel.push(&in_value);
  scheduler.run_once();
  EXPECT_FALSE(task.done());
  EXPECT_EQ(in_channel.size(), 1);
  EXPECT_TRUE(out_channel.empty());
  EXPECT_NE(in_value.get_revents() & skynet::reactor::kPollOut, 0);

  executor.run_once();
  EXPECT_TRUE(task.done());
  EXPECT_NO_THROW(task.get());

  scheduler.run_once();
  EXPECT_TRUE(in_channel.empty());
}

TEST(network_scheduler, reschedule) {
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
  argo::schedulers::network_scheduler scheduler(out_channel, in_channel);

  ASSERT_EQ(sender.bind(address), skynet::bind_status::kSuccess);
  ASSERT_EQ(receiver.bind(address), skynet::bind_status::kSuccess);
  receiver.getsockname(address);
  receiver.set_option(skynet::options::nonblock(true));
  reactor.insert_or_assign(receiver, skynet::reactor::kPollIn, nullptr);

  auto task = [](skynet::inet::udp::socket * socket, std::span<std::byte> bytes,
                 skynet::inet::sockaddr * address) -> argo::task<void> {
    EXPECT_EQ(co_await argo::awaiters::recv(*socket, bytes.first(1)), 1);
    EXPECT_EQ(
        co_await argo::awaiters::recvfrom(*socket, bytes.last(1), *address), 1);
  }(&receiver, received, &peer_address);

  in_channel.push(task.get_execution_token());
  executor.run_once();

  for (std::size_t index = 0; index < bytes.size(); ++index) {
    EXPECT_FALSE(task.done());
    EXPECT_EQ(out_channel.size(), 1);

    scheduler.run_once();
    EXPECT_TRUE(in_channel.empty());
    ASSERT_EQ(sender.sendto(std::span(bytes).subspan(index, 1), address), 1);
    ASSERT_FALSE(reactor.poll(std::chrono::seconds(1)).empty());

    scheduler.run_once();
    EXPECT_EQ(in_channel.size(), 1);

    executor.run_once();
  }

  EXPECT_TRUE(task.done());
  EXPECT_NO_THROW(task.get());
  EXPECT_EQ(received, bytes);
  sender.getsockname(address);
  EXPECT_EQ(peer_address, address);
}

TEST(network_scheduler, full_out_channel) {
  skynet::inet::tcp::socket acceptor;
  skynet::inet::tcp::socket client;
  skynet::inet::sockaddr address(skynet::inet::ip::kLoopback(),
                                 skynet::inet::port(0));
  skynet::reactor reactor(1);

  flow::spsc_queue<argo::execution_token> in_channel(1);
  flow::spsc_queue<argo::schedulers::network_in_value *> out_channel(1);
  argo::executors::inline_executor executor(in_channel, out_channel);
  argo::schedulers::network_scheduler scheduler(out_channel, in_channel);

  ASSERT_EQ(acceptor.bind(address), skynet::bind_status::kSuccess);
  ASSERT_EQ(client.bind(address), skynet::bind_status::kSuccess);
  acceptor.getsockname(address);
  acceptor.listen(1);
  acceptor.close();
  client.set_option(skynet::options::nonblock(true));
  reactor.insert_or_assign(client, skynet::reactor::kPollOut, nullptr);

  auto task = [](skynet::inet::tcp::socket * socket,
                 skynet::inet::sockaddr const * address)
      -> argo::task<skynet::connect_status> {
    co_return co_await argo::awaiters::connect(*socket, *address);
  }(&client, &address);
  auto blocker = []() -> argo::task<void> { co_return; }();

  in_channel.push(task.get_execution_token());
  executor.run_once();
  ASSERT_FALSE(reactor.poll(std::chrono::seconds(1)).empty());

  in_channel.push(blocker.get_execution_token());
  scheduler.run_once();
  EXPECT_FALSE(task.done());
  EXPECT_EQ(in_channel.size(), 1);
  EXPECT_TRUE(out_channel.empty());

  scheduler.run_once();
  EXPECT_EQ(in_channel.size(), 1);

  executor.run_once();
  EXPECT_TRUE(blocker.done());

  scheduler.run_once();
  EXPECT_EQ(in_channel.size(), 1);

  executor.run_once();
  EXPECT_TRUE(task.done());
  EXPECT_EQ(task.get(), skynet::connect_status::kRefused);

  scheduler.run_once();
  EXPECT_TRUE(in_channel.empty());
}

TEST(network_scheduler, full_out_channel_multiple_events) {
  skynet::inet::udp::socket socket1;
  skynet::inet::udp::socket socket2;
  skynet::inet::sockaddr const address(skynet::inet::ip::kLoopback(),
                                       skynet::inet::port(0));

  flow::spsc_queue<argo::execution_token> in_channel(1);
  flow::spsc_queue<argo::schedulers::network_in_value *> out_channel(2);
  argo::executors::inline_executor executor(in_channel, out_channel);
  argo::schedulers::network_scheduler scheduler(out_channel, in_channel);

  ASSERT_EQ(socket1.bind(address), skynet::bind_status::kSuccess);
  ASSERT_EQ(socket2.bind(address), skynet::bind_status::kSuccess);

  auto task1   = []() -> argo::task<void> { co_return; }();
  auto task2   = []() -> argo::task<void> { co_return; }();
  auto blocker = []() -> argo::task<void> { co_return; }();
  argo::schedulers::network_in_value value1(task1.get_execution_token(),
                                            socket1, skynet::reactor::kPollOut);
  argo::schedulers::network_in_value value2(task2.get_execution_token(),
                                            socket2, skynet::reactor::kPollOut);

  out_channel.push(&value1);
  out_channel.push(&value2);
  in_channel.push(blocker.get_execution_token());
  scheduler.run_once();
  EXPECT_FALSE(task1.done());
  EXPECT_FALSE(task2.done());
  EXPECT_EQ(in_channel.size(), 1);
  EXPECT_TRUE(out_channel.empty());

  executor.run_once();
  EXPECT_TRUE(blocker.done());

  scheduler.run_once();
  ASSERT_EQ(in_channel.size(), 1);
  executor.run_once();

  scheduler.run_once();
  ASSERT_EQ(in_channel.size(), 1);
  executor.run_once();

  EXPECT_TRUE(task1.done());
  EXPECT_TRUE(task2.done());

  scheduler.run_once();
  EXPECT_TRUE(in_channel.empty());
  EXPECT_TRUE(out_channel.empty());
}

TEST(network_scheduler, registration_exception) {
  skynet::inet::udp::socket socket;
  skynet::inet::sockaddr const address(skynet::inet::ip::kLoopback(),
                                       skynet::inet::port(0));

  flow::spsc_queue<argo::execution_token> in_channel(1);
  flow::spsc_queue<argo::schedulers::network_in_value *> out_channel(1);
  argo::executors::inline_executor executor(in_channel, out_channel);
  argo::schedulers::network_scheduler scheduler(out_channel, in_channel);

  socket.close();

  auto task = []() -> argo::task<void> { co_return; }();
  argo::schedulers::network_in_value in_value(
      task.get_execution_token(), socket, skynet::reactor::kPollOut);

  out_channel.push(&in_value);
  EXPECT_THROW(scheduler.run_once(), std::runtime_error);
  EXPECT_FALSE(task.done());
  EXPECT_TRUE(in_channel.empty());
  EXPECT_TRUE(out_channel.empty());

  socket = skynet::inet::udp::socket();
  ASSERT_EQ(socket.bind(address), skynet::bind_status::kSuccess);

  scheduler.run_once();
  EXPECT_EQ(in_channel.size(), 1);

  executor.run_once();
  EXPECT_TRUE(task.done());
  EXPECT_NO_THROW(task.get());
}

TEST(network_scheduler, dispatch_exception) {
  class channel final : public flow::spsc_queue<argo::execution_token> {
   public:
    using flow::spsc_queue<argo::execution_token>::spsc_queue;

    auto try_push(argo::execution_token && value) -> bool {
      if (std::exchange(fail_, false)) {
        throw std::runtime_error("try_push(): failed");
      }
      argo::execution_context::this_executor().resume(std::move(value));
      return true;
    }

   private:
    bool fail_ = true;
  };

  skynet::inet::udp::socket socket;
  skynet::inet::sockaddr const address(skynet::inet::ip::kLoopback(),
                                       skynet::inet::port(0));
  bool completed = false;

  channel in_channel(1);
  flow::spsc_queue<argo::schedulers::network_in_value *> out_channel(1);
  argo::executors::inline_executor executor(in_channel, out_channel);
  argo::schedulers::network_scheduler scheduler(out_channel, in_channel);

  ASSERT_EQ(socket.bind(address), skynet::bind_status::kSuccess);

  {
    auto task = [](skynet::inet::udp::socket * socket,
                   bool * completed) -> argo::task<void> {
      co_await argo::awaiters::network(
          *socket, skynet::reactor::kPollOut,
          [ready = false](auto) mutable -> std::optional<bool> {
            return std::exchange(ready, true) ? std::optional(true)
                                              : std::nullopt;
          });
      *completed = true;
    }(&socket, &completed);

    in_channel.push(task.get_execution_token());
    executor.run_once();
    EXPECT_FALSE(task.done());
  }

  EXPECT_THROW(scheduler.run_once(), std::runtime_error);
  EXPECT_FALSE(completed);
  EXPECT_TRUE(in_channel.empty());

  scheduler.run_once();
  EXPECT_TRUE(completed);

  scheduler.run_once();
  EXPECT_TRUE(in_channel.empty());
}

}  // namespace tests::unit::schedulers
