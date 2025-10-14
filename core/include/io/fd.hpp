#pragma once

#include "utils/static_pimpl.hpp"

namespace core::io {

/**
 * @brief Copy-featured RAII wrapper for a file descriptor
 *
 * The programmer is responsible for opening the native descriptor. The
 * constructor does still verifies whether the validity of a handed descriptor
 */
class fd {
 public:
  class uninitialized_t final {};

 private:
  friend class poller;  ///< Needed by poller to access native_handle()

 public:
  static const fd& kStdin() noexcept;
  static const fd& kStdout() noexcept;
  static const fd& kStderr() noexcept;

 protected:
  explicit fd(uninitialized_t) noexcept;
  static fd adopt(int fd) noexcept(false);

 public:
  fd(const fd& that) noexcept(false);
  fd& operator=(const fd& that) noexcept(false);
  fd(fd&& that) noexcept;
  fd& operator=(fd&& that) noexcept(false);
  ~fd() noexcept;

 public:
  /**
   * @brief Identity comparison
   *
   * File descriptor is only equal to itself i.e. operator== is the identity
   * comparison
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

 protected:
  int native_handle() const noexcept;

 private:
  class impl;
  utils::static_pimpl<impl, 4, 4> pimpl_;
};

}  // namespace core::io
