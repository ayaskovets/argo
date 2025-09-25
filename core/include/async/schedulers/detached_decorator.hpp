#pragma once

#include <thread>

#include "async/scheduler.hpp"

namespace core::async::schedulers {

template <async::concepts::scheduler Scheduler>
class detached_decorator : public Scheduler {
 public:
  constexpr void run_detached(std::stop_token stop_token) {
    auto worker_task =
        [stop_token]() noexcept(noexcept(Scheduler::run_once())) {
          while (!stop_token.stop_requested()) [[likely]] {
            Scheduler::run_once();
          }
        };

    worker_ = std::jthread(std::move(worker_task));
  }

 private:
  std::jthread worker_;
};

}  // namespace core::async::schedulers
