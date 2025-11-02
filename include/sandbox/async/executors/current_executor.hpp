#pragma once

#include <sandbox/async/channel.hpp>
#include <sandbox/async/scheduler.hpp>

#include <coroutine>
#include <functional>

namespace sandbox::async::executors::current_executor {

extern thread_local std::function<bool(const base_scheduler_input&)> try_push;

constexpr void suspend(const base_scheduler_input& params) {
  // TODO: handle channel overflow
  try_push(params);
  try_push = nullptr;
}

template <typename Executor>
constexpr void resume(Executor& executor, std::coroutine_handle<> handle) {
  try_push = [&executor](const base_scheduler_input& params) {
    using out_channel_t = std::decay_t<decltype(executor.out_channel())>;
    using out_channel_value_t = typename out_channel_t::value_type;

    // TODO: static_cast?
    return executor.out_channel().try_push(
        dynamic_cast<out_channel_value_t>(&params));
  };
  handle.resume();
}

}  // namespace sandbox::async::executors::current_executor
