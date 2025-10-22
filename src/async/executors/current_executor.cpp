#include <sandbox/async/executors/current_executor.hpp>

namespace sandbox::async::executirs {

std::function<bool(const base_scheduler_input&)> try_push = nullptr;

}  // namespace sandbox::async::executirs
