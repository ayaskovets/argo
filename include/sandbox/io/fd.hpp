#pragma once

#include <sandbox/io/uninitialized.hpp>
#include <sandbox/utils/static_pimpl.hpp>

#include <span>

namespace sandbox::io {

/**
 * @brief Copy-featured RAII wrapper for a file descriptor/handle
 *
 * The programmer is responsible for opening the native descriptor. The
 * constructor does still verifies whether the validity of a handed descriptor
 */
class fd {
 private:
  friend class poller;  ///< Needed by poller to access native_handle()

 public:
  static fd& kStdin() noexcept;
  static fd& kStdout() noexcept;
  static fd& kStderr() noexcept;

 protected:
  class impl;
  const impl& native_handle() const noexcept;

 protected:
  explicit fd(uninitialized_t) noexcept;
  static fd adopt(const impl& native_handle) noexcept(false);

 public:
  fd(const fd& that) noexcept(false);
  fd& operator=(const fd& that) noexcept(false);
  fd(fd&& that) noexcept;
  fd& operator=(fd&& that) noexcept(false);
  ~fd() noexcept;

 public:
  void set_nonblock(bool value) noexcept(false);

 public:
  bool get_nonblock() const noexcept(false);

 public:
  std::size_t write(std::span<const std::byte> bytes) noexcept(false);
  std::size_t read(std::span<std::byte> bytes) noexcept(false);

 public:
  /**
   * @brief Identity comparison
   *
   * File descriptor is only equal to itself i.e. operator== is identity
   * comparison
   *
   * An uninitialized or moved-from descriptor is equal to any other
   * descriptor in unitialized or moved-from state
   */
  bool operator==(const fd& that) const noexcept;
  bool operator!=(const fd& that) const noexcept;

 public:
  /**
   * @brief Manually close a file descriptor
   *
   * This method explicitly throws an exception in case of any failure. This is
   * done to keep RAII semantics for the type but to allow user code to receive
   * the error if needed
   *
   * Any subsequent call results in an exception
   */
  void close() noexcept(false);

 private:
  utils::static_pimpl<impl, 4, 4> pimpl_;
};

}  // namespace sandbox::io
