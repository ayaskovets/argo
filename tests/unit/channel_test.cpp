#include <argo/channel.hpp>
#include <argo/channels/multiplexed_channel.hpp>

#include <flow/mpmc_queue.hpp>
#include <flow/mpsc_queue.hpp>
#include <flow/spmc_queue.hpp>
#include <flow/spsc_queue.hpp>
#include <flow/unbounded_spsc_queue.hpp>
#include <gtest/gtest.h>

namespace tests::unit {

TEST(channel, flow) {
  static_assert(argo::concepts::channel<flow::mpmc_queue<int>>);
  static_assert(argo::concepts::noexcept_channel<flow::mpmc_queue<int>>);
  static_assert(argo::concepts::channel<flow::mpmc_queue<int>, int>);
  static_assert(argo::concepts::noexcept_channel<flow::mpmc_queue<int>, int>);

  static_assert(argo::concepts::channel<flow::mpsc_queue<int>>);
  static_assert(argo::concepts::noexcept_channel<flow::mpsc_queue<int>>);
  static_assert(argo::concepts::channel<flow::mpsc_queue<int>, int>);
  static_assert(argo::concepts::noexcept_channel<flow::mpsc_queue<int>, int>);

  static_assert(argo::concepts::channel<flow::spmc_queue<int>>);
  static_assert(argo::concepts::noexcept_channel<flow::spmc_queue<int>>);
  static_assert(argo::concepts::channel<flow::spmc_queue<int>, int>);
  static_assert(argo::concepts::noexcept_channel<flow::spmc_queue<int>, int>);

  static_assert(argo::concepts::channel<flow::spsc_queue<int>>);
  static_assert(argo::concepts::noexcept_channel<flow::spsc_queue<int>>);
  static_assert(argo::concepts::channel<flow::spsc_queue<int>, int>);
  static_assert(argo::concepts::noexcept_channel<flow::spsc_queue<int>, int>);

  static_assert(argo::concepts::channel<flow::unbounded_spsc_queue<int>>);
  static_assert(
      !argo::concepts::noexcept_channel<flow::unbounded_spsc_queue<int>>);
  static_assert(argo::concepts::channel<flow::unbounded_spsc_queue<int>, int>);
  static_assert(
      !argo::concepts::noexcept_channel<flow::unbounded_spsc_queue<int>, int>);
}

TEST(channel, multiplexed_channel) {
  using multiplexed_channel =
      argo::channels::multiplexed_channel<flow::spsc_queue<int>,
                                          flow::unbounded_spsc_queue<float>>;

  static_assert(argo::concepts::multichannel<multiplexed_channel, int, float>);
  static_assert(
      !argo::concepts::noexcept_multichannel<multiplexed_channel, int, float>);
  static_assert(
      argo::concepts::noexcept_typed_channel<multiplexed_channel, int>);

  static_assert(argo::concepts::channel<multiplexed_channel>);
  static_assert(!argo::concepts::noexcept_channel<multiplexed_channel>);
  static_assert(
      !argo::concepts::noexcept_typed_channel<multiplexed_channel, float>);
}

}  // namespace tests::unit
