#include "utils/leak_detector_allocator.hpp"

#include <gtest/gtest.h>

namespace tests::utils {

TEST(utils_leak_detector_allocator, smoke) {
  core::utils::leak_detector_allocator<int> allocator;

  {
    std::vector<int, decltype(allocator)> vector(allocator);
    EXPECT_TRUE(allocator.is_clean());

    vector.push_back(1);
    EXPECT_FALSE(allocator.is_clean());

    vector.push_back(2);
    EXPECT_FALSE(allocator.is_clean());

    vector.clear();
    EXPECT_FALSE(allocator.is_clean());
  }

  EXPECT_TRUE(allocator.is_clean());
}

}  // namespace tests::utils
