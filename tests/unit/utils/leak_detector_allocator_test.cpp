#include <sandbox/utils/leak_detector_allocator.hpp>

#include <gtest/gtest.h>

namespace tests::utils {

namespace utils = sandbox::utils;

TEST(utils_leak_detector_allocator, smoke) {
  utils::leak_detector_allocator<int> allocator;

  {
    EXPECT_TRUE(allocator.is_clean());

    const auto ptr = allocator.allocate(8);
    EXPECT_FALSE(allocator.is_clean());

    allocator.deallocate(ptr, 8);
    EXPECT_TRUE(allocator.is_clean());
  }

  {
    const auto result = allocator.allocate_at_least(8);
    EXPECT_FALSE(allocator.is_clean());

    allocator.deallocate(result.ptr, result.count);
    EXPECT_TRUE(allocator.is_clean());
  }

  EXPECT_TRUE(allocator.is_clean());
}

TEST(utils_leak_detector_allocator, shared) {
  utils::leak_detector_allocator<int, true> allocator;

  {
    std::vector<int, decltype(allocator)> vector(allocator);
    EXPECT_TRUE(allocator.is_clean());

    vector.push_back(1);
    EXPECT_FALSE(allocator.is_clean());

    utils::leak_detector_allocator<int, true> shared_copy;
    EXPECT_FALSE(shared_copy.is_clean());

    utils::leak_detector_allocator<int> copy;
    EXPECT_TRUE(copy.is_clean());
  }

  EXPECT_TRUE(allocator.is_clean());
}

}  // namespace tests::utils
