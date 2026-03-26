#pragma once

#include <wip/io/fd.hpp>

#include <swiss/strong_typedef.hpp>

namespace wip::io {

class fd::impl final : public swiss::strong_typedef<class fd_impl, int> {
  using swiss::strong_typedef<int>::strong_typedef;
};

}  // namespace wip::io
