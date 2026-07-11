#pragma once

#include <swiss/mixins.hpp>

#include "../channel.hpp"
#include "../this_executor.hpp"

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

  using in_value_type  = typename InChannel::value_type;
  using out_value_type = typename OutChannel::value_type;

 public:
  constexpr inline_executor(InChannel & in_channel,
                            OutChannel & out_channel) noexcept
      : in_channel_(in_channel), out_channel_(out_channel) {}

 public:
  constexpr auto in_channel() const noexcept -> InChannel & {
    return in_channel_;
  }
  constexpr auto out_channel() const noexcept -> OutChannel & {
    return out_channel_;
  }

 public:
  /**
   * @brief Run the next task if there is any in the input channel
   * @return True if a task was executed
   */
  constexpr auto run_once() noexcept(concepts::noexcept_channel<InChannel>)
      -> bool {
    if (in_value_type in_value{}; in_channel_.try_pop(in_value)) {
      this_executor::resume(*this, std::move(in_value));
      return true;
    }

    return false;
  }

  /**
   * @brief Block until at least one task from the input channel gets executed
   * or stop is requested
   * @return True if a task was executed
   */
  constexpr auto run_once(std::stop_token const & stop_token) noexcept(
      concepts::noexcept_channel<InChannel>) -> bool {
    in_value_type in_value{};
    if (!in_channel_.pop(in_value, stop_token)) {
      return false;
    }

    this_executor::resume(*this, std::move(in_value));
    return true;
  }

 private:
  // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
  InChannel & in_channel_;
  OutChannel & out_channel_;
  // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)
};

}  // namespace argo::executors
