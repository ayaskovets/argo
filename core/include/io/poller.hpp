#pragma once

#include <functional>

#include "io/fd.hpp"
#include "utils/mixins.hpp"
#include "utils/static_pimpl.hpp"

namespace core::io {

/**
 * @brief File descriptor events poller
 *
 * Does not own file descriptors and a close() or another operation that
 * invalidates the descriptor would be noticeable only after poll() on the
 * invalidated descriptor
 */
class poller final : utils::non_copyable {
 public:
  enum event : std::uint8_t {
    kPollIn = 1 << 0,
    kPollOut = 1 << 1,
    kPollErr = 1 << 2,
    kPollHup = 1 << 3,
    kPollNval = 1 << 4,
  };
  using events_t = std::uint8_t;
  using callback_t = std::function<void(const io::fd&, events_t)>;

 public:
  explicit poller(callback_t callback) noexcept;
  poller(poller&&) noexcept;
  poller& operator=(poller&&) noexcept;
  ~poller() noexcept;

 public:
  /**
   * @brief Track events on the passed descriptor
   *
   * Returns true if the fd was inserted
   *
   * Safe to call from the callback
   */
  bool insert_or_assign(const io::fd& fd, events_t events);

  /**
   * @brief Track events on the passed descriptor
   *
   * Returns true if the fd was erased
   *
   * Safe to call from the callback
   */
  bool erase(const io::fd& fd);

 public:
  /**
   * @brief Synchronously poll for the events on tracked file descriptors
   *
   * Return the number of unhandled errors during the event processing
   *
   * Any exceptions thrown from the callback function will be silently dropped
   * and accounted in the return value
   */
  std::size_t try_poll(std::chrono::milliseconds timeout);
  std::size_t poll();

 private:
  struct impl;
  utils::static_pimpl<impl, 88, 8> pimpl_;
  callback_t callback_;
};

}  // namespace core::io
