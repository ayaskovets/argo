#pragma once

#include <swiss/sealable_stack.hpp>

#include "execution_token.hpp"

namespace argo {

/**
 * @brief Lock-free append-only stack
 */
using task_waitlist = swiss::sealable_stack<
    std::pair<execution_token, class execution_context const *>>;

}  // namespace argo
