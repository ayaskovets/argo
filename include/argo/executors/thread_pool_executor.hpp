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
  using in_value_type  = typename InChannel::value_type;
  using out_value_type = typename OutChannel::value_type;

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
    workers_.clear();
    for (std::size_t i = 0; i < workers_.capacity(); ++i) {
      workers_.emplace_back([this, &stop_token]() { worker_task(stop_token); });
    }
  }

 private:
  // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
  InChannel & in_channel_;
  OutChannel & out_channel_;
  // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)

 private:
  std::vector<std::jthread> workers_;

 private:
  constexpr void worker_task(std::stop_token const & stop_token) noexcept {
    in_value_type in_value{};
    while (true) {
      /**
       * @note Noexcept not to fail worker threads in case of an exception
       * while resuming a task
       */
      try {
        if (!in_channel_.pop(stop_token, in_value)) {
          return;
        }

        this_executor::resume(*this, std::move(in_value));
      } catch (std::exception const &) {
        // TODO: bugprone-empty-catch
      }
    }
  }
};

}  // namespace argo::executors
