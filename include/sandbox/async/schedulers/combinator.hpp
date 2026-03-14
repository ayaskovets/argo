#pragma once

#include <wip/async/scheduler.hpp>

#include <tuple>
#include <utility>

namespace wip::async::schedulers {

/**
 * @brief Combinator for sequential execution of run_once() for multiple
 * schedulers
 */
template <async::concepts::scheduler... Schedulers>
class combinator final {
 public:
  constexpr explicit combinator(Schedulers&&... schedulers) noexcept
      : schedulers_(std::forward<Schedulers>(schedulers)...) {}

 public:
  constexpr void run_once() noexcept(
      (noexcept(std::declval<Schedulers>().run_once()) && ...)) {
    std::apply([](auto&&... scheduler) { (scheduler.run_once(), ...); },
               schedulers_);
  }

  constexpr void run_detached(std::stop_token stop_token) noexcept(
      (noexcept(std::declval<Schedulers>().run_detached()) && ...)) {
    std::apply(
        [stop_token](auto&&... scheduler) {
          (scheduler.run_detached(stop_token), ...);
        },
        schedulers_);
  }

 private:
  std::tuple<Schedulers...> schedulers_;
};

}  // namespace wip::async::schedulers
