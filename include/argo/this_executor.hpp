#pragma once

#include "execution_context.hpp"

/**
 * @brief TLS execution context
 */
namespace argo {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
extern thread_local execution_context this_executor;

}  // namespace argo
