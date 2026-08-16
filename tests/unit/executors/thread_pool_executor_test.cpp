#include <argo/executors/thread_pool_executor.hpp>
#include <argo/task.hpp>

#include <stop_token>

#include <flow/spsc_queue.hpp>
#include <gtest/gtest.h>

namespace tests::unit::awaiters {

TEST(thread_pool_executor, smoke) {
  std::stop_source stop_source;

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
  auto task = [](std::stop_source & stop_source) -> argo::task<void> {
    stop_source.request_stop();
    co_return;
  }(stop_source);

  flow::spsc_queue<argo::task_ref> in_channel(1);
  flow::spsc_queue<std::nullptr_t> out_channel(1);
  argo::executors::thread_pool_executor executor(in_channel, out_channel);

  in_channel.push(argo::task_ref(task));
  executor.run_detached(stop_source.get_token());
}

}  // namespace tests::unit::awaiters
