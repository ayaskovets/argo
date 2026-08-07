#pragma once

#include <thread>
#include <vector>

#include <swiss/mixins.hpp>

#include "../channel.hpp"
#include "../execution_context.hpp"

namespace argo::executors {

/**
 * @brief Thread pool executor
 */
template <concepts::channel InChannel, concepts::channel OutChannel>
class thread_pool_executor final : public swiss::non_copyable,
                                   public swiss::non_movable {
 public:
  using in_channel_type  = InChannel;
  using out_channel_type = OutChannel;

 public:
  constexpr thread_pool_executor(
      in_channel_type & in_channel, out_channel_type & out_channel,
      std::size_t workers_size = std::thread::hardware_concurrency()) noexcept
      : in_channel_(in_channel), out_channel_(out_channel), ctx_(*this) {
    workers_.reserve(workers_size);
  }

 public:
  constexpr auto in_channel() const noexcept -> in_channel_type & {
    return in_channel_;
  }
  constexpr auto out_channel() const noexcept -> out_channel_type & {
    return out_channel_;
  }
  constexpr auto context() const noexcept -> execution_context & {
    return ctx_;
  }

 public:
  constexpr void run_detached(std::stop_token const & stop_token) {
    workers_.clear();
    for (std::size_t i = 0; i < workers_.capacity(); ++i) {
      workers_.emplace_back(
          [this, &stop_token]() noexcept -> void { worker_task(stop_token); });
    }
  }

 private:
  // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
  in_channel_type & in_channel_;
  out_channel_type & out_channel_;
  // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)
  execution_context ctx_;
  std::vector<std::jthread> workers_;

 private:
  constexpr void worker_task(std::stop_token const & stop_token) noexcept {
    typename in_channel_type::value_type in_value{};
    while (true) {
      /**
       * @note Noexcept not to fail worker threads in case of an exception
       * while resuming a task
       */
      try {
        if (!in_channel_.pop(stop_token, in_value)) {
          break;
        }
        // NOLINTNEXTLINE(bugprone-empty-catch)
      } catch (...) {
      }

      ctx_.resume(std::move(in_value));
    }
  }
};

}  // namespace argo::executors
