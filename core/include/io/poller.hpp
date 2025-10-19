#pragma once

#include <chrono>
#include <functional>

#include "io/base_fd.hpp"
#include "utils/mixins.hpp"
#include "utils/static_pimpl.hpp"

namespace core::io {

// TODO: rewrite poller interface and communication with base_fd

/**
 * @brief File descriptor events poller
 *
 * Does not own file descriptors and a close() or another operation that
 * invalidates the descriptor would be noticeable only after poll() on the
 * invalidated descriptor
 */
class poller final : public utils::non_copyable {
 public:
  enum event : std::uint8_t {
    kPollIn = 1 << 0,
    kPollPri = 1 << 1,
    kPollOut = 1 << 2,
    kPollErr = 1 << 3,
    kPollHup = 1 << 4,
    kPollNval = 1 << 5,
  };
  using events_t = std::uint8_t;
  using callback_t = std::function<void(const io::base_fd&, events_t)>;

 public:
  explicit poller(callback_t callback) noexcept;
  poller(poller&&) noexcept;
  poller& operator=(poller&&) noexcept;
  ~poller() noexcept;

 public:
  /**
   * @brief Track events on the passed descriptor
   *
   * Returns true if the base_fd was inserted
   *
   * Safe to call from the callback
   */
  bool insert_or_assign(const io::base_fd& base_fd,
                        events_t events) noexcept(false);

  /**
   * @brief Track events on the passed descriptor
   *
   * Returns true if the base_fd was erased
   *
   * Safe to call from the callback
   */
  bool erase(const io::base_fd& base_fd) noexcept;

 public:
  /**
   * @brief Synchronously poll for the events on tracked file descriptors
   *
   * Return the number of unhandled errors during the event processing
   *
   * Any exceptions thrown from the callback function will be silently dropped
   * and accounted in the return value
   */
  template <typename Duration>
  constexpr std::size_t poll_for(Duration timeout) noexcept(false) {
    return poll_for(static_cast<std::size_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(timeout)
            .count()));
  }
  std::size_t try_poll() noexcept(false);
  std::size_t poll() noexcept(false);

 private:
  struct impl;
  utils::static_pimpl<impl, 88, 8> pimpl_;
  callback_t callback_;

 private:
  std::size_t poll_for(std::size_t milliseconds) noexcept(false);
};

}  // namespace core::io
