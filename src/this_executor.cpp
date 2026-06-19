#include <argo/this_executor.hpp>

namespace argo::this_executor {

thread_local std::function<bool(
    std::chrono::steady_clock::time_point time_point,
    std::coroutine_handle<> const handle)>
    try_push = nullptr;

}  // namespace argo::this_executor
