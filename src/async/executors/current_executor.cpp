#include <wip/async/executors/current_executor.hpp>

namespace wip::async::executirs {

std::function<bool(const base_scheduler_input&)> try_push = nullptr;

}  // namespace wip::async::executirs
