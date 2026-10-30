#include <argo/promise_ctrl.hpp>
#include <argo/promise_ptr.hpp>

#include <gtest/gtest.h>

namespace tests::unit {

TEST(promise_ptr, smoke) {
  argo::promise_ctrl ctrl(nullptr);
  {
    argo::promise_ptr<int> const ptr1(ctrl);
    EXPECT_EQ(ctrl.use_count(), 1);

    {
      argo::promise_ptr const ptr2(ctrl);
      EXPECT_EQ(ctrl.use_count(), 2);
    }

    EXPECT_EQ(ctrl.use_count(), 1);
  }
  EXPECT_EQ(ctrl.use_count(), 0);
}

TEST(promise_ptr, copy) {
  argo::promise_ctrl ctrl(nullptr);
  {
    argo::promise_ptr const ptr(ctrl);

    argo::promise_ptr copy(ptr);
    EXPECT_EQ(ctrl.use_count(), 2);

    copy = ptr;
    EXPECT_EQ(ctrl.use_count(), 2);
  }
  EXPECT_EQ(ctrl.use_count(), 0);
}

TEST(promise_ptr, copy_assignment) {
  argo::promise_ctrl ctrl1(nullptr);
  argo::promise_ctrl ctrl2(nullptr);

  {
    argo::promise_ptr const ptr1(ctrl1);
    argo::promise_ptr const ptr2(ctrl2);
    argo::promise_ptr const empty;

    {
      argo::promise_ptr copy(ptr1);
      EXPECT_EQ(ctrl1.use_count(), 2);
      EXPECT_EQ(ctrl2.use_count(), 1);

      copy = ptr2;
      EXPECT_EQ(ctrl1.use_count(), 1);
      EXPECT_EQ(ctrl2.use_count(), 2);

      copy = ptr2;
      EXPECT_EQ(ctrl1.use_count(), 1);
      EXPECT_EQ(ctrl2.use_count(), 2);

      copy = empty;
      EXPECT_FALSE(copy);
      EXPECT_EQ(ctrl2.use_count(), 1);

      copy = ptr2;
      EXPECT_TRUE(copy);
      EXPECT_EQ(ctrl2.use_count(), 2);
    }

    EXPECT_EQ(ctrl1.use_count(), 1);
    EXPECT_EQ(ctrl2.use_count(), 1);
  }

  EXPECT_EQ(ctrl1.use_count(), 0);
  EXPECT_EQ(ctrl2.use_count(), 0);
}

TEST(promise_ptr, move) {
  argo::promise_ctrl ctrl(nullptr);
  {
    argo::promise_ptr ptr(ctrl);

    argo::promise_ptr move(std::move(ptr));
    EXPECT_EQ(ctrl.use_count(), 1);

    move = std::move(ptr);
    EXPECT_EQ(ctrl.use_count(), 1);
  }
  EXPECT_EQ(ctrl.use_count(), 0);
}

}  // namespace tests::unit
