#pragma once

#include <sandbox/io/fd.hpp>
#include <sandbox/utils/mixins.hpp>
#include <sandbox/utils/static_pimpl.hpp>

#include <chrono>
#include <functional>
#include <type_traits>

namespace sandbox::io {

// TODO: rewrite poller interface and communication with fd

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
  using events_t = std::underlying_type_t<event>;
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
  bool insert_or_assign(const io::fd& fd, events_t events) noexcept(false);

  /**
   * @brief Track events on the passed descriptor
   *
   * Returns true if the fd was erased
   *
   * Safe to call from the callback
   */
  bool erase(const io::fd& fd) noexcept;

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

}  // namespace sandbox::io
