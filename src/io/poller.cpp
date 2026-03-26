#include <wip/io/fd_impl.hpp>
#include <wip/io/poller.hpp>
#include <wip/utils/syscall.hpp>

#include <chrono>
#include <format>
#include <poll.h>
#include <unordered_map>
#include <vector>

namespace wip::io {

namespace {

constexpr short kEmptyEvents = 0;

constexpr short to_native_events(poller::events_t events) noexcept {
  short native_events = kEmptyEvents;
  native_events |= (events & poller::event::kPollIn) ? POLLIN : 0;
  native_events |= (events & poller::event::kPollPri) ? POLLPRI : 0;
  native_events |= (events & poller::event::kPollOut) ? POLLOUT : 0;
  native_events |= (events & poller::event::kPollErr) ? POLLERR : 0;
  native_events |= (events & poller::event::kPollHup) ? POLLHUP : 0;
  native_events |= (events & poller::event::kPollNval) ? POLLNVAL : 0;
  return native_events;
}

constexpr poller::events_t from_native_events(short native_events) noexcept {
  poller::events_t events = kEmptyEvents;
  events |= (native_events & POLLIN) ? poller::event::kPollIn : 0;
  events |= (native_events & POLLPRI) ? poller::event::kPollPri : 0;
  events |= (native_events & POLLOUT) ? poller::event::kPollOut : 0;
  events |= (native_events & POLLERR) ? poller::event::kPollErr : 0;
  events |= (native_events & POLLHUP) ? poller::event::kPollHup : 0;
  events |= (native_events & POLLNVAL) ? poller::event::kPollNval : 0;
  return events;
}

}  // namespace

struct poller::impl final {
  // TODO: allocator
  std::vector<::pollfd> fds;
  std::unordered_map<int, std::size_t> fds_index_by_native_handle;
  std::vector<::pollfd> fds_to_insert;
};

poller::poller(callback_t callback) noexcept : callback_(std::move(callback)) {}

poller::poller(poller && that) noexcept = default;

poller & poller::operator=(poller && that) noexcept = default;

poller::~poller() noexcept = default;

bool poller::insert_or_assign(io::fd const & fd,
                              events_t events) noexcept(false) {
  auto & [fds, fds_index_by_native_handle, fds_to_insert] = *pimpl_;

  auto fds_it = fds_index_by_native_handle.find(*fd.native_handle());
  if (fds_it != fds_index_by_native_handle.end()) [[unlikely]] {
    fds.at(fds_it->second).events = to_native_events(events);
    return false;
  }

  auto fds_to_insert_it =
      std::find_if(fds_to_insert.begin(), fds_to_insert.end(),
                   [&fd](::pollfd const & pollfd) {
                     return pollfd.fd == *fd.native_handle();
                   });
  if (fds_to_insert_it != fds_to_insert.end()) [[unlikely]] {
    fds_to_insert_it->events = to_native_events(events);
    return false;
  }

  ::pollfd pollfd{
      .fd = *fd.native_handle(),
      .events = to_native_events(events),
      .revents = kEmptyEvents,
  };

  if (fds.size() < fds.capacity()) [[likely]] {
    fds_index_by_native_handle.emplace(pollfd.fd, fds.size());
    fds.push_back(std::move(pollfd));
  } else {
    fds_to_insert.push_back(std::move(pollfd));
  }
  return true;
}

bool poller::erase(io::fd const & fd) noexcept {
  auto & [fds, fds_index_by_native_handle, fds_to_insert] = *pimpl_;

  auto it = fds_index_by_native_handle.find(*fd.native_handle());
  if (it != fds_index_by_native_handle.end()) [[likely]] {
    fds.at(it->second).fd = -fds.at(it->second).fd;
    fds_index_by_native_handle.erase(it);
    return true;
  }

  auto fds_to_insert_it =
      std::find_if(fds_to_insert.begin(), fds_to_insert.end(),
                   [&fd](::pollfd const & pollfd) {
                     return pollfd.fd == *fd.native_handle();
                   });
  if (fds_to_insert_it != fds_to_insert.end()) [[unlikely]] {
    fds_to_insert_it->fd = -fds_to_insert_it->fd;
    return true;
  }

  return false;
}

std::size_t poller::try_poll() noexcept(false) {
  constexpr std::chrono::milliseconds kNonBlockingPollTimeout(0);

  return poll_for(kNonBlockingPollTimeout);
}

std::size_t poller::poll() noexcept(false) {
  constexpr std::chrono::milliseconds kBlockingTimeout(-1);

  return poll_for(kBlockingTimeout);
}

std::size_t poller::poll_for(std::size_t milliseconds) noexcept(false) {
  constexpr int kSyscallError = -1;

  auto & [fds, fds_index_by_native_handle, fds_to_insert] = *pimpl_;

  if (!fds_to_insert.empty()) [[unlikely]] {
    fds.reserve(fds.size() + fds_to_insert.size());
    for (std::size_t i = 0; i < fds_to_insert.size(); ++i) {
      fds_index_by_native_handle.emplace(fds_to_insert.at(i).fd, fds.size());
      fds.push_back(std::move(fds_to_insert.at(i)));
    }
    fds_to_insert.clear();
  }

  auto retval = SYSCALL(::poll(fds.data(), fds.size(), milliseconds));
  if (retval == kSyscallError) [[unlikely]] {
    throw std::runtime_error(
        std::format("poll() failed: {}", std::strerror(errno)));
  }

  std::size_t unhandled = 0;
  for (std::size_t i = 0; i < fds.size() && retval > 0; ++i) {
    if (fds.at(i).fd < 0) [[unlikely]] {
      fds_index_by_native_handle.at(fds.back().fd) = i;
      fds.at(i) = fds.back();
      fds.pop_back();
      i -= 1;
      continue;
    }

    if (fds.at(i).revents == kEmptyEvents) {
      continue;
    }

    if (!callback_) [[unlikely]] {
      unhandled += 1;
    } else {
      try {
        // TODO: remove reinterpret_cast
        callback_(
            *std::launder(reinterpret_cast<io::fd const *>(&fds.at(i).fd)),
            from_native_events(fds.at(i).revents));
      } catch (...) {
        unhandled += 1;
      }
    }
    retval -= 1;
  }
  return unhandled;
}

}  // namespace wip::io
