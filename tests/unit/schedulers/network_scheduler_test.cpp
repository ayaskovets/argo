#include <argo/awaiters/io_awaiter.hpp>
#include <argo/executor.hpp>
#include <argo/executors/inline_executor.hpp>
#include <argo/executors/thread_pool_executor.hpp>
#include <argo/net/inet/sockaddr.hpp>
#include <argo/net/inet/tcp/socket.hpp>
#include <argo/schedulers/io_scheduler.hpp>
#include <argo/schedulers/timer_scheduler.hpp>
#include <argo/task.hpp>

#include <flow/lockfree_mpmc_queue.hpp>
#include <gtest/gtest.h>

using namespace std::chrono_literals;

namespace tests::async {

using in_channel_value_type =
    async::schedulers::detail::io_scheduler_input<std::coroutine_handle<>>;
using out_channel_value_type = std::coroutine_handle<>;

using io_channel_type = flow::lockfree_mpmc_queue<in_channel_value_t const *>;
using handle_channel_type = flow::lockfree_mpmc_queue<out_channel_value_t>;

using scheduler_type =
    async::schedulers::io_scheduler<io_channel_t, handle_channel_t>;
using executor_type =
    async::executors::inline_executor<handle_channel_t, io_channel_t>;

async::task<int> server(argo::net::inet::tcp::socket & srv) {
  std::cerr << std::chrono::system_clock::now() << " - server() ENTER"
            << std::endl;

  std::vector<argo::net::inet::tcp::socket> peers(10,
                                                  argo::io::uninitialized_t{});
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

async::task<int> client(argo::net::inet::tcp::socket * cnt_ptr,
                        argo::net::inet::sockaddr const & server_sockaddr) {
  std::cerr << std::chrono::system_clock::now() << " - client() ENTER"
            << std::endl;

  for (std::size_t i = 0; i < 4; ++i) {
    argo::net::inet::tcp::socket cnt;
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

  io_channel_type io_channel(128);
  handle_channel_type handle_channel(128);
  scheduler_type scheduler(io_channel, handle_channel);
  executor_type executor(handle_channel, io_channel);

  static_assert(async::concepts::executor<executor_type>);

  auto const server_sockaddr = argo::net::inet::sockaddr(
      argo::net::inet::ip("127.0.0.1"), argo::net::inet::port(8081));

  argo::net::inet::tcp::socket srv;
  srv.set_flags(io::fd::flag::kNonblock);
  srv.set_reuseaddr(true);
  srv.set_reuseport(true);
  srv.bind(server_sockaddr);
  srv.listen(1);

  argo::net::inet::tcp::socket cnt;
  cnt.set_flags(io::fd::flag::kNonblock);
  cnt.set_reuseaddr(true);
  cnt.set_reuseport(true);

  auto server_task = server(srv);
  auto client_task = client(&cnt, server_sockaddr);

  in_channel_value_t const server_value(srv, argo::io::poller::event::kPollIn,
                                        server_task.handle());
  io_channel.push(&server_value);

  in_channel_value_t const client_value(cnt, argo::io::poller::event::kPollIn,
                                        client_task.handle());
  io_channel.push(&client_value);

  EXPECT_EQ(argo::net::connect_status::kPending, cnt.connect(server_sockaddr));

  auto const done_at = std::chrono::steady_clock::now() + 1000ms;
  while (std::chrono::steady_clock::now() < done_at) {
    scheduler.run_once();
    executor.run_once();
  }

  std::cerr << std::chrono::system_clock::now() << " - end test" << std::endl;
}

}  // namespace tests::async
