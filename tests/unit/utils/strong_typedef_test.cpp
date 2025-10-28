#include <sandbox/utils/strong_typedef.hpp>

#include <gtest/gtest.h>

namespace tests::utils {

namespace utils = sandbox::utils;

TEST(utils_strong_typedef, size) {
  const utils::strong_typedef<std::string> tdef;

  static_assert(sizeof(decltype(tdef)) == sizeof(std::string));
  static_assert(alignof(decltype(tdef)) == alignof(std::string));
}

TEST(utils_strong_typedef, smoke) {
  std::shared_ptr<int> ptr = std::make_shared<int>(1);
  {
    EXPECT_EQ(ptr.use_count(), 1);
    const utils::strong_typedef<std::shared_ptr<int>> tdef(ptr);
    EXPECT_EQ(ptr.use_count(), 2);
    EXPECT_EQ((*tdef).get(), ptr.get());
    EXPECT_EQ(tdef->get(), ptr.get());
  }
  EXPECT_EQ(ptr.use_count(), 1);
}

TEST(utils_strong_typedef, copy) {
  std::shared_ptr<int> ptr = std::make_shared<int>(1);
  {
    const utils::strong_typedef<std::shared_ptr<int>> tdef(ptr);
    EXPECT_EQ(ptr.use_count(), 2);
    utils::strong_typedef<std::shared_ptr<int>> copy(tdef);
    EXPECT_EQ(ptr.use_count(), 3);
    EXPECT_EQ(tdef->get(), ptr.get());
    EXPECT_EQ(tdef->get(), copy->get());
  }
  EXPECT_EQ(ptr.use_count(), 1);
}

TEST(utils_strong_typedef, copy_assignment) {
  std::shared_ptr<int> ptr = std::make_shared<int>(1);
  {
    const utils::strong_typedef<std::shared_ptr<int>> tdef(ptr);
    EXPECT_EQ(ptr.use_count(), 2);
    utils::strong_typedef<std::shared_ptr<int>> copy;
    copy = tdef;
    EXPECT_EQ(ptr.use_count(), 3);
    EXPECT_EQ(tdef->get(), ptr.get());
    EXPECT_EQ(tdef->get(), copy->get());
  }
  EXPECT_EQ(ptr.use_count(), 1);
}

TEST(utils_strong_typedef, value_copy_assignment) {
  utils::strong_typedef<std::string> tdef("a string");
  EXPECT_EQ(*tdef, "a string");
  const std::string value("another string");
  *tdef = value;
  EXPECT_EQ(*tdef, "another string");
}

TEST(utils_strong_typedef, move) {
  std::shared_ptr<int> ptr = std::make_shared<int>(1);
  {
    utils::strong_typedef<std::shared_ptr<int>> tdef(ptr);
    EXPECT_EQ(ptr.use_count(), 2);
    const utils::strong_typedef<std::shared_ptr<int>> moved_to(std::move(tdef));
    EXPECT_EQ(ptr.use_count(), 2);
    EXPECT_EQ(moved_to->get(), ptr.get());
  }
  EXPECT_EQ(ptr.use_count(), 1);
}

TEST(utils_strong_typedef, move_assignment) {
  std::shared_ptr<int> ptr = std::make_shared<int>(1);
  {
    utils::strong_typedef<std::shared_ptr<int>> tdef(ptr);
    EXPECT_EQ(ptr.use_count(), 2);
    utils::strong_typedef<std::shared_ptr<int>> moved_to;
    moved_to = std::move(tdef);
    EXPECT_EQ(ptr.use_count(), 2);
    EXPECT_EQ(moved_to->get(), ptr.get());
  }
  EXPECT_EQ(ptr.use_count(), 1);
}

TEST(utils_strong_typedef, value_move_assignment) {
  utils::strong_typedef<std::string> tdef("a string");
  EXPECT_EQ(*tdef, "a string");
  *tdef = std::string("another string");
  EXPECT_EQ(*tdef, "another string");
}

}  // namespace tests::utils
