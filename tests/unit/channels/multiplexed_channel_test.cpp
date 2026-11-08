#include <argo/channels/multiplexed_channel.hpp>

#include <flow/spsc_queue.hpp>
#include <gtest/gtest.h>

namespace tests::unit {

TEST(multiplexed_channel, smoke) {
  // NOLINTBEGIN(readability-magic-numbers)
  argo::channels::multiplexed_channel<flow::spsc_queue<int>,
                                      flow::spsc_queue<float>>
      multichannel(2, 2);

  int ivalue{};
  float fvalue{};

  multichannel.emplace<int>(1);
  EXPECT_TRUE(multichannel.emplace<int>(std::stop_token{}, 2));
  EXPECT_FALSE(multichannel.try_emplace<int>(3));

  multichannel.emplace<float>(4.F);
  EXPECT_TRUE(multichannel.emplace<float>(std::stop_token{}, 5.F));
  EXPECT_FALSE(multichannel.try_emplace<float>(6.F));

  multichannel.pop(ivalue);
  EXPECT_EQ(ivalue, 1);

  EXPECT_TRUE(multichannel.pop(std::stop_token{}, ivalue));
  EXPECT_EQ(ivalue, 2);
  EXPECT_FALSE(multichannel.try_pop(ivalue));

  multichannel.pop(fvalue);
  EXPECT_EQ(fvalue, 4.F);

  EXPECT_TRUE(multichannel.pop(std::stop_token{}, fvalue));
  EXPECT_EQ(fvalue, 5.F);
  EXPECT_FALSE(multichannel.try_pop(fvalue));
  // NOLINTEND(readability-magic-numbers)
}

}  // namespace tests::unit
