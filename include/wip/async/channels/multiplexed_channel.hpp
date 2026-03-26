#pragma once

#include <wip/async/channel.hpp>
#include <wip/async/scheduler.hpp>

#include <tuple>

namespace wip::async::channels {

template <async::concepts::channel... Channels>
class multiplexed_channel final {
 public:
  using value_type = const base_scheduler_input*;

 public:
  constexpr explicit multiplexed_channel(Channels&&... channels) noexcept
      : channels_(std::forward<Channels>(channels)...) {}

 public:
  constexpr bool try_push(value_type value) noexcept {
    static_assert(
        (std::is_base_of_v<base_scheduler_input,
                           std::remove_pointer_t<
                               typename std::decay_t<Channels>::value_type>> ||
         ...));

    bool found = false;
    return ([&]() {
      using underlying_t = typename std::decay_t<Channels>::value_type;

      if (found) {
        return false;
      }

      if (const underlying_t underlying = dynamic_cast<underlying_t>(value);
          underlying) {
        found = true;
        return std::get<Channels>(channels_).try_push(underlying);
      }

      return false;
    }() || ...);
  }

  template <typename Value>
  constexpr bool try_pop(const Value*& value) noexcept {
    static_assert((std::is_same_v<typename std::decay_t<Channels>::value_type,
                                  const Value*> ||
                   ...));

    return ([&]() {
      if constexpr (std::is_same_v<typename std::decay_t<Channels>::value_type,
                                   const Value*>) {
        return std::get<Channels>(channels_).try_pop(value);
      }

      return false;
    }() || ...);
  }

 private:
  std::tuple<Channels...> channels_;
};

}  // namespace wip::async::channels
