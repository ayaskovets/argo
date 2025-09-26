#pragma once

#include <memory>
#include <unordered_map>

namespace core::utils {

template <typename T>
class leak_detector_allocator : public std::allocator<T> {
 public:
  using std::allocator<T>::allocator;

 public:
  constexpr T* allocate(std::size_t n) {
    T* ptr = std::allocator<T>::allocate(n);
    allocations_by_ptr_[ptr] += n;
    return ptr;
  }

  constexpr std::allocation_result<T*> allocate_at_least(std::size_t n) {
    return std::allocation_result<T*>{.ptr = allocate(n), .count = n};
  }

  constexpr void deallocate(T* ptr, std::size_t n) {
    if ((allocations_by_ptr_.at(ptr) -= n) == 0) {
      allocations_by_ptr_.erase(ptr);
    }
    std::allocator<T>::deallocate(ptr, n);
  }

 public:
  template <typename To>
  class rebind {
   public:
    typedef leak_detector_allocator<To> other;
  };

 public:
  static constexpr bool is_clean() { return allocations_by_ptr_.empty(); }

 private:
  static inline std::unordered_map<void*, std::size_t> allocations_by_ptr_;
};

}  // namespace core::utils
