#include <wip/utils/unchecked_any.hpp>

#include <memory>
#include <utility>

#include <gtest/gtest.h>

namespace tests::utils {

namespace utils = wip::utils;

TEST(utils_type_erased_pimpl, size) {
  static_assert(sizeof(utils::unchecked_any) == 24);
  static_assert(alignof(utils::unchecked_any) == 8);
}

TEST(utils_type_erased_pimpl, construction) {
  const utils::unchecked_any inplace_pimpl(std::in_place_type<std::string>,
                                           "abc");
  EXPECT_EQ(inplace_pimpl.value<std::string>(), "abc");
  EXPECT_EQ(inplace_pimpl.get<std::string>().size(), 3);

  utils::unchecked_any pimpl(std::string("abc"));
  EXPECT_EQ(pimpl.value<std::string>(), "abc");
  EXPECT_EQ(pimpl.get<std::string>().size(), 3);

  pimpl = std::string("abc");
  EXPECT_EQ(pimpl.value<std::string>(), "abc");
  EXPECT_EQ(pimpl.get<std::string>().size(), 3);
}

TEST(utils_type_erased_pimpl, smoke) {
  std::shared_ptr<int> ptr = std::make_shared<int>(1);
  {
    EXPECT_EQ(ptr.use_count(), 1);
    const utils::unchecked_any pimpl(ptr);
    EXPECT_EQ(ptr.use_count(), 2);
    EXPECT_EQ(pimpl.get<std::shared_ptr<int>>(), ptr);
    EXPECT_EQ(pimpl.value<std::shared_ptr<int>>(), ptr);
  }
  EXPECT_EQ(ptr.use_count(), 1);
}

TEST(utils_type_erased_pimpl, copy) {
  std::shared_ptr<int> ptr = std::make_shared<int>(1);
  {
    const utils::unchecked_any pimpl(ptr);
    EXPECT_EQ(ptr.use_count(), 2);
    utils::unchecked_any copy(pimpl);
    EXPECT_EQ(ptr.use_count(), 3);
    EXPECT_EQ(pimpl.get<std::shared_ptr<int>>(), ptr);
    EXPECT_EQ(pimpl.get<std::shared_ptr<int>>(),
              copy.get<std::shared_ptr<int>>());
  }
  EXPECT_EQ(ptr.use_count(), 1);
}

TEST(utils_type_erased_pimpl, copy_assignment) {
  std::shared_ptr<int> ptr = std::make_shared<int>(1);
  {
    const utils::unchecked_any pimpl(ptr);
    EXPECT_EQ(ptr.use_count(), 2);
    utils::unchecked_any copy(42);
    copy = pimpl;
    EXPECT_EQ(ptr.use_count(), 3);
    EXPECT_EQ(pimpl.get<std::shared_ptr<int>>(), ptr);
    EXPECT_EQ(pimpl.get<std::shared_ptr<int>>(),
              copy.get<std::shared_ptr<int>>());
  }
  EXPECT_EQ(ptr.use_count(), 1);
}

TEST(utils_type_erased_pimpl, value_copy_assignment) {
  utils::unchecked_any pimpl(std::string("a string"));
  EXPECT_EQ(pimpl.get<std::string>(), "a string");
  EXPECT_EQ(pimpl.value<std::string>(), "a string");
  const std::string value("another string");
  pimpl = value;
  EXPECT_EQ(pimpl.get<std::string>(), "another string");
  EXPECT_EQ(pimpl.value<std::string>(), "another string");
}

TEST(utils_type_erased_pimpl, move) {
  std::shared_ptr<int> ptr = std::make_shared<int>(1);
  {
    utils::unchecked_any pimpl(ptr);
    EXPECT_EQ(ptr.use_count(), 2);
    EXPECT_TRUE(!pimpl.valueless_after_move());
    const utils::unchecked_any moved_to(std::move(pimpl));
    EXPECT_TRUE(pimpl.valueless_after_move());
    EXPECT_EQ(ptr.use_count(), 2);
    EXPECT_EQ(moved_to.get<std::shared_ptr<int>>(), ptr);
  }
  EXPECT_EQ(ptr.use_count(), 1);
}

TEST(utils_type_erased_pimpl, move_assignment) {
  std::shared_ptr<int> ptr = std::make_shared<int>(1);
  {
    utils::unchecked_any pimpl(ptr);
    EXPECT_EQ(ptr.use_count(), 2);
    utils::unchecked_any moved_to(42);
    moved_to = std::move(pimpl);
    EXPECT_EQ(ptr.use_count(), 2);
    EXPECT_EQ(moved_to.get<std::shared_ptr<int>>(), ptr);
  }
  EXPECT_EQ(ptr.use_count(), 1);
}

TEST(utils_type_erased_pimpl, value_move_assignment) {
  utils::unchecked_any pimpl(std::string("a string"));
  EXPECT_EQ(pimpl.get<std::string>(), "a string");
  pimpl = std::string("another string");
  EXPECT_EQ(pimpl.get<std::string>(), "another string");
}

}  // namespace tests::utils
