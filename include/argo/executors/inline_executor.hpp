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
  constexpr inline_executor(InChannel & in_channel,
                            OutChannel & out_channel) noexcept
      : in_channel_(in_channel), out_channel_(out_channel) {}

 public:
  constexpr auto in_channel() noexcept -> InChannel & { return in_channel_; }
  constexpr auto out_channel() noexcept -> OutChannel & { return out_channel_; }

 public:
  constexpr void run_once() noexcept {
    typename InChannel::value_type task{};
    if (in_channel_.try_pop(task)) {
      this_executor::resume(*this, task);
    }
  }

 private:
  // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
  InChannel & in_channel_;
  OutChannel & out_channel_;
  // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)
};

}  // namespace argo::executors
