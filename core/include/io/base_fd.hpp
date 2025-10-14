#pragma once

#include "io/uninitialized.hpp"
#include "utils/static_pimpl.hpp"

namespace core::io {

/**
 * @brief Copy-featured RAII wrapper for a file descriptor
 *
 * The programmer is responsible for opening the native descriptor. The
 * constructor does still verifies whether the validity of a handed descriptor
 */
class base_fd {
 private:
  friend class poller;  ///< Needed by poller to access native_handle()

 public:
  static const base_fd& kStdin() noexcept;
  static const base_fd& kStdout() noexcept;
  static const base_fd& kStderr() noexcept;

 protected:
  explicit base_fd(uninitialized_t) noexcept;
  static base_fd adopt(int base_fd) noexcept(false);

 public:
  base_fd(const base_fd& that) noexcept(false);
  base_fd& operator=(const base_fd& that) noexcept(false);
  base_fd(base_fd&& that) noexcept;
  base_fd& operator=(base_fd&& that) noexcept(false);
  ~base_fd() noexcept;

 public:
  /**
   * @brief Identity comparison
   *
   * File descriptor is only equal to itself i.e. operator== is the identity
   * comparison
   */
  bool operator==(const base_fd& that) const noexcept;
  bool operator!=(const base_fd& that) const noexcept;

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
