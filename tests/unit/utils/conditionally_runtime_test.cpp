#include <sandbox/utils/conditionally_runtime.hpp>

#include <gtest/gtest.h>

namespace tests::utils {

TEST(utils_conditionally_runtime, size) {
  static_assert(
      sizeof(sandbox::utils::conditionally_runtime<std::size_t, false, 42>) ==
      1);
  static_assert(
      sizeof(sandbox::utils::conditionally_runtime<std::size_t, true, 42>) ==
      8);
  static_assert(
      alignof(sandbox::utils::conditionally_runtime<std::size_t, false, 42>) ==
      1);
  static_assert(
      alignof(sandbox::utils::conditionally_runtime<std::size_t, true, 42>) ==
      8);
}

TEST(utils_conditionally_runtime, constructor) {
  sandbox::utils::conditionally_runtime<std::size_t, false, 42>();
  sandbox::utils::conditionally_runtime<std::size_t, true>(42);
}

TEST(utils_conditionally_runtime, dereference) {
  {
    sandbox::utils::conditionally_runtime<int, false, 1> v;
    static_assert(std::is_same_v<decltype(*v), const int&>);
    EXPECT_EQ(*v, 1);
  }
  {
    sandbox::utils::conditionally_runtime<int, true> v(3);
    static_assert(std::is_same_v<decltype(*v), int&>);
    EXPECT_EQ(*v, 3);
  }
  {
    const sandbox::utils::conditionally_runtime<int, true> v(3);
    static_assert(std::is_same_v<decltype(*v), const int&>);
    EXPECT_EQ(*v, 3);
  }
}

TEST(utils_conditionally_runtime, member_access) {
  {
    sandbox::utils::conditionally_runtime<std::pair<int, int>, false,
                                          std::pair<int, int>{0, 0}>
        v;
    EXPECT_EQ(v->first, 0);
  }
  {
    sandbox::utils::conditionally_runtime<std::pair<int, int>, true,
                                          std::pair<int, int>{0, 0}>
        v(std::pair<int, int>{1, 1});

    EXPECT_EQ(v->second, 1);
  }
}

TEST(utils_conditionally_runtime, assignment) {
  {
    sandbox::utils::conditionally_runtime<int, true> v(4);
    EXPECT_EQ(*v, 4);
    *v = 5;
    EXPECT_EQ(*v, 5);
  }
  {
    sandbox::utils::conditionally_runtime<int, true> v1(4);
    sandbox::utils::conditionally_runtime<int, true> v2(5);
    EXPECT_EQ(*v1, 4);
    v1 = v2;
    EXPECT_EQ(*v1, 5);
  }
}

}  // namespace tests::utils
