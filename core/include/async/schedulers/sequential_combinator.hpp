#pragma once

#include <utility>

#include "async/scheduler.hpp"

namespace core::async::schedulers {

template <async::concepts::scheduler... Schedulers>
class sequential_combinator final {
 public:
  constexpr explicit sequential_combinator(Schedulers&&... schedulers) noexcept
      : schedulers_(std::forward<Schedulers>(schedulers)...) {}

 public:
  constexpr void run_once() noexcept(
      (noexcept(std::declval<Schedulers>().run_once()) && ...)) {
    std::apply([](auto&&... scheduler) { (scheduler.run_once(), ...); },
               schedulers_);
  }

 private:
  std::tuple<Schedulers...> schedulers_;
};

}  // namespace core::async::schedulers
