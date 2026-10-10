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
      // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
      argo::promise_ptr<int> const ptr2(ctrl);
      EXPECT_EQ(ctrl.use_count(), 2);
    }

    EXPECT_EQ(ctrl.use_count(), 1);
  }
  EXPECT_EQ(ctrl.use_count(), 0);
}

TEST(promise_ptr, copy) {
  argo::promise_ctrl ctrl(nullptr);
  {
    argo::promise_ptr<int> const ptr(ctrl);

    argo::promise_ptr<int> copy(ptr);
    EXPECT_EQ(ctrl.use_count(), 2);

    copy = ptr;
    EXPECT_EQ(ctrl.use_count(), 2);
  }
  EXPECT_EQ(ctrl.use_count(), 0);
}

TEST(promise_ptr, move) {
  argo::promise_ctrl ctrl(nullptr);
  {
    argo::promise_ptr<int> ptr(ctrl);

    argo::promise_ptr<int> move(std::move(ptr));
    EXPECT_EQ(ctrl.use_count(), 1);

    move = std::move(ptr);
    EXPECT_EQ(ctrl.use_count(), 1);
  }
  EXPECT_EQ(ctrl.use_count(), 0);
}

}  // namespace tests::unit
