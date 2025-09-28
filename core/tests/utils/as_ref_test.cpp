#include "utils/as_ref.hpp"

#include <gtest/gtest.h>

namespace tests::utils {

TEST(utils_as_ref, pointer) {
  int value = 42;

  const int* const const_ptr = &value;
  EXPECT_EQ(core::utils::as_ref(const_ptr) + 1, 43);
  EXPECT_EQ(core::utils::as_ref(&const_ptr), const_ptr);

  int* const ptr = &value;
  EXPECT_EQ(core::utils::as_ref(ptr) + 1, 43);
  EXPECT_EQ(core::utils::as_ref(&ptr), ptr);
}

TEST(utils_as_ref, lvalue_reference) {
  const int const_value = 42;
  EXPECT_EQ(core::utils::as_ref(const_value) + 1, 43);
  EXPECT_EQ(core::utils::as_ref(&const_value), const_value);

  int value = 42;
  EXPECT_EQ(core::utils::as_ref(value) + 1, 43);
  EXPECT_EQ(core::utils::as_ref(&value), value);
}

TEST(utils_as_ref, rvalue_reference) {
  EXPECT_EQ(core::utils::as_ref(42) + 1, 43);
}

}  // namespace tests::utils
