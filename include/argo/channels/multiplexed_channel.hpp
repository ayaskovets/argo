#pragma once

#include <argo/channel.hpp>
#include <argo/scheduler.hpp>

#include <tuple>

#include <swiss/concepts.hpp>

namespace argo::channels {

template <concepts::channel... Channels>
class multiplexed_channel final {
 public:
  using value_types = std::variant<typename Channels::value_type...>;
  static_assert(swiss::unique_variant<value_types>);

 public:
  explicit constexpr multiplexed_channel(auto &&... args)
      : channels_(std::forward<decltype(args)>(args)...) {}

 public:
  template <typename Value, typename... Args>
    requires(std::constructible_from<Value, Args...>)
  constexpr auto try_emplace(Args &&... args) noexcept(noexcept(
      demultiplex<Value>().try_emplace(std::forward<decltype(args)>(args)...)))
      -> bool {
    return demultiplex<Value>().try_emplace(
        std::forward<decltype(args)>(args)...);
  }

  template <typename Value>
  constexpr auto try_push(Value && value) noexcept(noexcept(
      demultiplex<Value>().try_push(std::forward<Value>(value)))) -> bool {
    return demultiplex<Value>().try_push(std::forward<Value>(value));
  }

  template <typename Value>
  constexpr auto try_pop(Value & value) noexcept(
      noexcept(demultiplex<Value>().try_pop(value))) -> bool {
    return demultiplex<Value>().try_pop(value);
  }

 public:
  template <typename Value, typename... Args>
    requires(std::constructible_from<Value, Args...>)
  constexpr auto emplace(Args &&... args) noexcept(noexcept(
      demultiplex<Value>().emplace(std::forward<decltype(args)>(args)...)))
      -> void {
    demultiplex<Value>().emplace(std::forward<decltype(args)>(args)...);
  }

  template <typename Value>
  constexpr auto push(Value && value) noexcept(
      noexcept(demultiplex<Value>().push(std::forward<Value>(value)))) -> void {
    demultiplex<Value>().push(std::forward<Value>(value));
  }

  template <typename Value>
  constexpr auto pop(Value & value) noexcept(
      noexcept(demultiplex<Value>().pop(value))) -> void {
    demultiplex<Value>().pop(value);
  }

 public:
  template <typename Value, typename... Args>
    requires(std::constructible_from<Value, Args...>)
  constexpr auto
  emplace(std::stop_token const & stop_token, Args &&... args) noexcept(
      noexcept(demultiplex<Value>().emplace(
          stop_token, std::forward<decltype(args)>(args)...))) -> bool {
    return demultiplex<Value>().emplace(stop_token,
                                        std::forward<decltype(args)>(args)...);
  }

  template <typename Value>
  constexpr auto
  push(std::stop_token const & stop_token, Value && value) noexcept(noexcept(
      demultiplex<Value>().push(stop_token, std::forward<Value>(value))))
      -> bool {
    return demultiplex<Value>().push(stop_token, std::forward<Value>(value));
  }

  template <typename Value>
  constexpr auto pop(
      std::stop_token const & stop_token,
      Value & value) noexcept(noexcept(demultiplex<Value>().pop(stop_token,
                                                                value)))
      -> bool {
    return demultiplex<Value>().pop(stop_token, value);
  }

 private:
  template <typename Value>
  constexpr auto demultiplex() noexcept -> auto & {
    return std::get<
        swiss::index_in_pack<Value, typename Channels::value_type...>>(
        channels_);
  }

 private:
  std::tuple<Channels...> channels_;
};

}  // namespace argo::channels
