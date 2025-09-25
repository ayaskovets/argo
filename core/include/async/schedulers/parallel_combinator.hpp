#pragma once

#include <utility>

#include "async/scheduler.hpp"

namespace core::async::schedulers {

template <async::concepts::scheduler... Schedulers>
class parallel_combinator final {
 public:
  constexpr explicit parallel_combinator(Schedulers&&... schedulers) noexcept
      : schedulers_(std::forward<Schedulers>(schedulers)...) {}

 public:
  constexpr void run_detached(std::stop_token stop_token) {
    std::apply(
        [stop_token](auto&&... scheduler) {
          (scheduler.run_detached(stop_token), ...);
        },
        schedulers_);
  }

 private:
  std::tuple<Schedulers...> schedulers_;
};

}  // namespace core::async::schedulers
