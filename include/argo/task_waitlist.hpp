#pragma once

#include <swiss/sealable_stack.hpp>

namespace argo {

using task_waitlist = swiss::sealable_stack<
    std::pair<class execution_token *, class execution_context const *>>;

}  // namespace argo
