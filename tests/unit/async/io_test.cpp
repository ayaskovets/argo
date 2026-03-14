#include <wip/async/awaiters/io_awaiter.hpp>
#include <wip/async/executor.hpp>
#include <wip/async/executors/inline_executor.hpp>
#include <wip/async/executors/thread_pool_executor.hpp>
#include <wip/async/schedulers/io_scheduler.hpp>
#include <wip/async/schedulers/timer_scheduler.hpp>
#include <wip/async/task.hpp>
#include <wip/net/inet/sockaddr.hpp>
#include <wip/net/inet/tcp/socket.hpp>
#include <wip/queues/lockfree_mpmc_queue.hpp>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

namespace tests::async {

namespace async = wip::async;

using in_channel_value_t =
    async::schedulers::detail::io_scheduler_input<std::coroutine_handle<>>;
using out_channel_value_t = std::coroutine_handle<>;

using io_channel_t =
    wip::queues::lockfree_mpmc_queue<const in_channel_value_t*>;
using handle_channel_t =
    wip::queues::lockfree_mpmc_queue<out_channel_value_t>;

using scheduler_t =
    async::schedulers::io_scheduler<io_channel_t, handle_channel_t>;
using executor_t =
    async::executors::inline_executor<handle_channel_t, io_channel_t>;

async::task<int> server(wip::net::inet::tcp::socket& srv) {
  std::cerr << std::chrono::system_clock::now() << " - server() ENTER"
            << std::endl;

  std::vector<wip::net::inet::tcp::socket> peers(
      10, wip::io::uninitialized_t{});
  for (std::size_t i = 0; i < 4; ++i) {
    std::cerr << std::chrono::system_clock::now() << " - server() ITER"
              << std::endl;

    co_await async::awaiters::accept(srv, peers[i]);

    std::cerr << std::chrono::system_clock::now() << " - server() ACCEPTED"
              << std::endl;

    std::vector<std::byte> bytes;
    bytes.resize(4);

    co_await async::awaiters::send(peers[i], std::span(bytes));

    std::cerr << std::chrono::system_clock::now() << " - server() SENT"
              << std::endl;
  }

  co_return 0;
}

async::task<int> client(wip::net::inet::tcp::socket* cnt_ptr,
                        const wip::net::inet::sockaddr& server_sockaddr) {
  std::cerr << std::chrono::system_clock::now() << " - client() ENTER"
            << std::endl;

  for (std::size_t i = 0; i < 4; ++i) {
    wip::net::inet::tcp::socket cnt;
    cnt.set_flags(io::fd::flag::kNonblock);
    cnt.set_reuseaddr(true);
    cnt.set_reuseport(true);

    if (!cnt_ptr) {
      cnt_ptr = &cnt;
    }

    std::cerr << std::chrono::system_clock::now() << " - client() ITER"
              << std::endl;

    co_await async::awaiters::connect(*cnt_ptr, server_sockaddr);

    std::cerr << std::chrono::system_clock::now() << " - client() CONNECTED"
              << std::endl;

    std::vector<std::byte> bytes{};
    bytes.resize(4);

    co_await async::awaiters::receive(*cnt_ptr, std::span(bytes));

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

  static_assert(async::concepts::executor<executor_t>);

  const auto server_sockaddr = wip::net::inet::sockaddr(
      wip::net::inet::ip("127.0.0.1"), wip::net::inet::port(8081));

  wip::net::inet::tcp::socket srv;
  srv.set_flags(io::fd::flag::kNonblock);
  srv.set_reuseaddr(true);
  srv.set_reuseport(true);
  srv.bind(server_sockaddr);
  srv.listen(1);

  wip::net::inet::tcp::socket cnt;
  cnt.set_flags(io::fd::flag::kNonblock);
  cnt.set_reuseaddr(true);
  cnt.set_reuseport(true);

  auto server_task = server(srv);
  auto client_task = client(&cnt, server_sockaddr);

  const in_channel_value_t server_value(
      srv, wip::io::poller::event::kPollIn, server_task.get_handle());
  io_channel.push(&server_value);

  const in_channel_value_t client_value(
      cnt, wip::io::poller::event::kPollIn, client_task.get_handle());
  io_channel.push(&client_value);

  EXPECT_EQ(wip::net::connect_status::kPending,
            cnt.connect(server_sockaddr));

  const auto done_at = std::chrono::steady_clock::now() + 1000ms;
  while (std::chrono::steady_clock::now() < done_at) {
    scheduler.run_once();
    executor.run_once();
  }

  std::cerr << std::chrono::system_clock::now() << " - end test" << std::endl;
}

}  // namespace tests::async
