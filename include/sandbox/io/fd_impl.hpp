#pragma once

#include <wip/io/fd.hpp>
#include <wip/utils/strong_typedef.hpp>

namespace wip::io {

class fd::impl final : public utils::strong_typedef<int> {
  using utils::strong_typedef<int>::strong_typedef;
};

}  // namespace wip::io
