#pragma once

#include <thread>
#include <vector>

#include <swiss/mixins.hpp>

#include "../channel.hpp"
#include "../this_executor.hpp"

namespace argo::executors {

/**
 * @brief Thread pool executor
 */
template <concepts::channel InChannel, concepts::channel OutChannel>
class thread_pool_executor final : public swiss::non_copyable,
                                   public swiss::non_movable {
 public:
  constexpr thread_pool_executor(
      InChannel & in_channel, OutChannel & out_channel,
      std::size_t workers_size = std::thread::hardware_concurrency()) noexcept
      : in_channel_(in_channel), out_channel_(out_channel) {
    workers_.reserve(workers_size);
  }

 public:
  constexpr auto in_channel() const noexcept -> InChannel & {
    return in_channel_;
  }
  constexpr auto out_channel() const noexcept -> OutChannel & {
    return out_channel_;
  }

 public:
  constexpr void run_detached(std::stop_token const & stop_token) {
    auto const worker_task =
        [this, &stop_token] noexcept(
            std::is_nothrow_invocable_v<typename InChannel::value_type>) {
          while (!stop_token.stop_requested()) {
            typename InChannel::value_type task{};
            if (in_channel_.try_pop(task)) {
              this_executor::resume(*this, std::move(task));
            }
          }
        };

    workers_.clear();
    for (std::size_t i = 0; i < workers_.capacity(); ++i) {
      workers_.emplace_back(worker_task);
    }
  }

 private:
  // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
  InChannel & in_channel_;
  OutChannel & out_channel_;
  // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)

 private:
  std::vector<std::jthread> workers_;
};

}  // namespace argo::executors
