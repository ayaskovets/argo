#pragma once

#include <sandbox/async/executors/current_executor.hpp>
#include <sandbox/utils/mixins.hpp>

#include <coroutine>
#include <thread>
#include <vector>

namespace sandbox::async::executors {

template <typename InChannel, typename OutChannel>
class thread_pool_executor final : public utils::non_copyable,
                                   public utils::non_movable {
 private:
  static_assert(
      std::is_same_v<typename InChannel::value_t, std::coroutine_handle<>>);

 public:
  constexpr thread_pool_executor(
      InChannel& in_channel, OutChannel& out_channel,
      std::size_t workers_size = std::thread::hardware_concurrency()) noexcept
      : in_channel_(in_channel), out_channel_(out_channel) {
    workers_.reserve(workers_size);
  }

 public:
  InChannel& in_channel() noexcept { return in_channel_; }
  OutChannel& out_channel() noexcept { return out_channel_; }

 public:
  constexpr void run_detached(std::stop_token stop_token) {
    const auto worker_task = [this, stop_token] {
      while (!stop_token.stop_requested()) {
        std::coroutine_handle<> handle;
        if (!in_channel_.try_pop(handle)) {
          continue;
        }
        // TODO: customize for regular callbacks
        // TODO: handle exception here
        current_executor::resume(*this, handle);
      }
    };

    for (std::size_t i = 0; i < workers_.capacity(); ++i) {
      workers_.emplace_back(worker_task);
    }
  }

 private:
  InChannel& in_channel_;
  OutChannel& out_channel_;

 private:
  std::vector<std::jthread> workers_;
  std::stop_token stop_;
};

}  // namespace sandbox::async::executors
