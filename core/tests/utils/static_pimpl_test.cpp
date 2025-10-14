#include "utils/static_pimpl.hpp"

#include <gtest/gtest.h>

namespace tests::utils {

TEST(utils_static_pimpl, size) {
  class alignas(16) impl final {};
  const core::utils::static_pimpl<impl, 16, 16> pimpl;

  static_assert(sizeof(decltype(pimpl)) == sizeof(impl));
  static_assert(alignof(decltype(pimpl)) == alignof(impl));
}

TEST(utils_static_pimpl, smoke) {
  std::shared_ptr<int> ptr = std::make_shared<int>(1);
  {
    EXPECT_EQ(ptr.use_count(), 1);
    const core::utils::static_pimpl<std::shared_ptr<int>, 16, 8> pimpl(ptr);
    EXPECT_EQ(ptr.use_count(), 2);
    EXPECT_EQ((*pimpl).get(), ptr.get());
    EXPECT_EQ(pimpl->get(), ptr.get());
  }
  EXPECT_EQ(ptr.use_count(), 1);
}

TEST(utils_static_pimpl, copy) {
  std::shared_ptr<int> ptr = std::make_shared<int>(1);
  {
    const core::utils::static_pimpl<std::shared_ptr<int>, 16, 8> pimpl(ptr);
    EXPECT_EQ(ptr.use_count(), 2);
    core::utils::static_pimpl<std::shared_ptr<int>, 16, 8> copy(pimpl);
    EXPECT_EQ(ptr.use_count(), 3);
    EXPECT_EQ(pimpl->get(), ptr.get());
    EXPECT_EQ(pimpl->get(), copy->get());
  }
  EXPECT_EQ(ptr.use_count(), 1);
}

TEST(utils_static_pimpl, copy_assignment) {
  std::shared_ptr<int> ptr = std::make_shared<int>(1);
  {
    const core::utils::static_pimpl<std::shared_ptr<int>, 16, 8> pimpl(ptr);
    EXPECT_EQ(ptr.use_count(), 2);
    core::utils::static_pimpl<std::shared_ptr<int>, 16, 8> copy;
    copy = pimpl;
    EXPECT_EQ(ptr.use_count(), 3);
    EXPECT_EQ(pimpl->get(), ptr.get());
    EXPECT_EQ(pimpl->get(), copy->get());
  }
  EXPECT_EQ(ptr.use_count(), 1);
}

TEST(utils_static_pimpl, move) {
  std::shared_ptr<int> ptr = std::make_shared<int>(1);
  {
    core::utils::static_pimpl<std::shared_ptr<int>, 16, 8> pimpl(ptr);
    EXPECT_EQ(ptr.use_count(), 2);
    const core::utils::static_pimpl<std::shared_ptr<int>, 16, 8> moved_to(
        std::move(pimpl));
    EXPECT_EQ(ptr.use_count(), 2);
    EXPECT_EQ(moved_to->get(), ptr.get());
  }
  EXPECT_EQ(ptr.use_count(), 1);
}

TEST(utils_static_pimpl, move_assignment) {
  std::shared_ptr<int> ptr = std::make_shared<int>(1);
  {
    core::utils::static_pimpl<std::shared_ptr<int>, 16, 8> pimpl(ptr);
    EXPECT_EQ(ptr.use_count(), 2);
    core::utils::static_pimpl<std::shared_ptr<int>, 16, 8> moved_to;
    moved_to = std::move(pimpl);
    EXPECT_EQ(ptr.use_count(), 2);
    EXPECT_EQ(moved_to->get(), ptr.get());
  }
  EXPECT_EQ(ptr.use_count(), 1);
}

TEST(utils_static_pimpl, emplace) {
  core::utils::static_pimpl<std::string, 24, 8> pimpl("a string");
  EXPECT_EQ(*pimpl, "a string");

  pimpl.emplace("another string");
  EXPECT_EQ(*pimpl, "another string");
}

}  // namespace tests::utils
