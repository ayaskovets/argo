#include <argo/this_executor.hpp>

namespace argo::this_executor::detail {
// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)

thread_local void const * this_executor = nullptr;

thread_local void (*yield)(std::coroutine_handle<> handle) = nullptr;

thread_local void (*park)(void const * awaiter,
                          std::type_info const & type_info) = nullptr;

// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)
}  // namespace argo::this_executor::detail
