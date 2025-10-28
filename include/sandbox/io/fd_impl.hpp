#pragma once

#include <sandbox/io/fd.hpp>
#include <sandbox/utils/strong_typedef.hpp>

namespace sandbox::io {

class fd::impl final : public utils::strong_typedef<int> {
  using utils::strong_typedef<int>::strong_typedef;
};

}  // namespace sandbox::io
