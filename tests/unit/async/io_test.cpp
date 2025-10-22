#include <sandbox/async/awaiters/io_awaiter.hpp>
#include <sandbox/async/executor.hpp>
#include <sandbox/async/executors/inline_executor.hpp>
#include <sandbox/async/executors/thread_pool_executor.hpp>
#include <sandbox/async/schedulers/io_scheduler.hpp>
#include <sandbox/async/schedulers/timer_scheduler.hpp>
#include <sandbox/async/task.hpp>
#include <sandbox/net/inet/sockaddr.hpp>
#include <sandbox/net/inet/tcp/socket.hpp>
#include <sandbox/queues/lockfree_mpmc_queue.hpp>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

namespace tests::async {

using in_channel_value_t =
    sandbox::async::schedulers::detail::io_scheduler_input<
        std::coroutine_handle<>>;
using out_channel_value_t = std::coroutine_handle<>;

using io_channel_t =
    sandbox::queues::lockfree_mpmc_queue<const in_channel_value_t*>;
using handle_channel_t =
    sandbox::queues::lockfree_mpmc_queue<out_channel_value_t>;

using scheduler_t =
    sandbox::async::schedulers::io_scheduler<io_channel_t, handle_channel_t>;
using executor_t =
    sandbox::async::executors::inline_executor<handle_channel_t, io_channel_t>;

sandbox::async::task<int> server(sandbox::net::inet::tcp::socket& srv) {
  std::cerr << std::chrono::system_clock::now() << " - server() ENTER"
            << std::endl;

  std::vector<sandbox::net::inet::tcp::socket> peers(
      10, sandbox::io::uninitialized_t{});
  for (std::size_t i = 0; i < 4; ++i) {
    std::cerr << std::chrono::system_clock::now() << " - server() ITER"
              << std::endl;

    co_await sandbox::async::awaiters::accept(srv, peers[i]);

    std::cerr << std::chrono::system_clock::now() << " - server() ACCEPTED"
              << std::endl;

    std::vector<std::byte> bytes;
    bytes.resize(4);

    co_await sandbox::async::awaiters::send(peers[i], std::span(bytes));

    std::cerr << std::chrono::system_clock::now() << " - server() SENT"
              << std::endl;
  }

  co_return 0;
}

sandbox::async::task<int> client(
    sandbox::net::inet::tcp::socket* cnt_ptr,
    const sandbox::net::inet::sockaddr& server_sockaddr) {
  std::cerr << std::chrono::system_clock::now() << " - client() ENTER"
            << std::endl;

  for (std::size_t i = 0; i < 4; ++i) {
    sandbox::net::inet::tcp::socket cnt;
    cnt.set_nonblock(true);
    cnt.set_reuseaddr(true);
    cnt.set_reuseport(true);

    if (!cnt_ptr) {
      cnt_ptr = &cnt;
    }

    std::cerr << std::chrono::system_clock::now() << " - client() ITER"
              << std::endl;

    co_await sandbox::async::awaiters::connect(*cnt_ptr, server_sockaddr);

    std::cerr << std::chrono::system_clock::now() << " - client() CONNECTED"
              << std::endl;

    std::vector<std::byte> bytes{};
    bytes.resize(4);

    co_await sandbox::async::awaiters::receive(*cnt_ptr, std::span(bytes));

    cnt_ptr = nullptr;

    std::cerr << std::chrono::system_clock::now() << " - client() RCVD"
              << std::endl;
  }
  co_return 0;
}

TEST(async_task, io_smoke) {
  std::cerr << std::chrono::system_clock::now() << " - begin test" << std::endl;

  io_channel_t io_channel(128);
  handle_channel_t handle_channel(128);
  scheduler_t scheduler(io_channel, handle_channel);
  executor_t executor(handle_channel, io_channel);

  static_assert(sandbox::async::concepts::executor<executor_t>);

  const auto server_sockaddr = sandbox::net::inet::sockaddr(
      sandbox::net::inet::ip("127.0.0.1"), sandbox::net::inet::port(8081));

  sandbox::net::inet::tcp::socket srv;
  srv.set_nonblock(true);
  srv.set_reuseaddr(true);
  srv.set_reuseport(true);
  srv.bind(server_sockaddr);
  srv.listen(1);

  sandbox::net::inet::tcp::socket cnt;
  cnt.set_nonblock(true);
  cnt.set_reuseaddr(true);
  cnt.set_reuseport(true);

  auto server_task = server(srv);
  auto client_task = client(&cnt, server_sockaddr);

  const in_channel_value_t server_value(
      srv, sandbox::io::poller::event::kPollIn, server_task.get_handle());
  io_channel.push(&server_value);

  const in_channel_value_t client_value(
      cnt, sandbox::io::poller::event::kPollIn, client_task.get_handle());
  io_channel.push(&client_value);

  EXPECT_EQ(sandbox::net::connect_status::kPending,
            cnt.connect(server_sockaddr));

  const auto done_at = std::chrono::steady_clock::now() + 1000ms;
  while (std::chrono::steady_clock::now() < done_at) {
    scheduler.run_once();
    executor.run_once();
  }

  std::cerr << std::chrono::system_clock::now() << " - end test" << std::endl;
}

}  // namespace tests::async
