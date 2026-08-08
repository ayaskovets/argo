#pragma once

#include <swiss/mixins.hpp>

#include "../channel.hpp"
#include "../execution_context.hpp"

namespace argo::executors {

/**
 * @brief Executor analogous to std::this_thread
 */
template <concepts::channel InChannel, concepts::channel OutChannel>
class inline_executor final : public swiss::non_copyable,
                              public swiss::non_movable {
 public:
  using in_channel_type  = InChannel;
  using out_channel_type = OutChannel;

 public:
  constexpr inline_executor(in_channel_type & in_channel,
                            out_channel_type & out_channel) noexcept
      : ctx_(*this), in_channel_(in_channel), out_channel_(out_channel) {}

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
  /**
   * @brief Run the next task if there is any in the input channel
   * @return True if a task was executed
   */
  constexpr auto run_once() noexcept(
      concepts::noexcept_channel<in_channel_type>) -> bool {
    typename in_channel_type::value_type in_value{};
    if (!in_channel_.try_pop(in_value)) {
      return false;
    }

    ctx_.resume(std::move(in_value));
    return true;
  }

  /**
   * @brief Block until at least one task from the input channel gets executed
   * or stop is requested
   * @return True if a task was executed
   */
  constexpr auto run_once(std::stop_token const & stop_token) noexcept(
      concepts::noexcept_channel<in_channel_type>) -> bool {
    typename in_channel_type::value_type in_value{};
    if (!in_channel_.pop(in_value, stop_token)) {
      return false;
    }

    ctx_.resume(std::move(in_value));
    return true;
  }

 private:
  mutable execution_context ctx_;

  // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
  in_channel_type & in_channel_;
  out_channel_type & out_channel_;
  // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)
};

}  // namespace argo::executors
