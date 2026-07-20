#include <argo/this_executor.hpp>

namespace argo {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
thread_local execution_context this_executor;

}  // namespace argo
