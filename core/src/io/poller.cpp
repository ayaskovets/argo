#include "io/poller.hpp"

#include <poll.h>

#include <format>
#include <unordered_map>
#include <vector>

namespace core::io {

namespace {

constexpr int kSyscallError = -1;
constexpr short kEmptyEvents = 0;
constexpr std::chrono::milliseconds kBlockingTimeout(-1);

constexpr short to_native_events(poller::events_t events) noexcept {
  short native_events = kEmptyEvents;
  native_events |= (events & poller::event::kPollIn) ? POLLIN : 0;
  native_events |= (events & poller::event::kPollOut) ? POLLOUT : 0;
  native_events |= (events & poller::event::kPollErr) ? POLLERR : 0;
  native_events |= (events & poller::event::kPollHup) ? POLLHUP : 0;
  native_events |= (events & poller::event::kPollNval) ? POLLNVAL : 0;
  return native_events;
}

constexpr poller::events_t from_native_events(short native_events) noexcept {
  poller::events_t events = kEmptyEvents;
  events |= (native_events & POLLIN) ? poller::event::kPollIn : 0;
  events |= (native_events & POLLOUT) ? poller::event::kPollOut : 0;
  events |= (native_events & POLLERR) ? poller::event::kPollErr : 0;
  events |= (native_events & POLLHUP) ? poller::event::kPollHup : 0;
  events |= (native_events & POLLNVAL) ? poller::event::kPollNval : 0;
  return events;
}

}  // namespace

struct poller::impl final {
  std::vector<::pollfd> fds;
  std::unordered_map<int, std::size_t> fds_index;
  std::vector<::pollfd> fds_to_insert;
};

poller::poller(callback_t callback) noexcept : callback_(std::move(callback)) {}

poller::poller(poller&& that) noexcept : pimpl_(std::move(that.pimpl_)) {}

poller& poller::operator=(poller&& that) noexcept {
  pimpl_ = std::move(that.pimpl_);
  return *this;
}

poller::~poller() noexcept {}

bool poller::insert_or_assign(const io::fd& fd, events_t events) {
  auto& [fds, fds_index, fds_to_insert] = *pimpl_;

  auto fds_it = fds_index.find(fd.native_handle());
  if (fds_it != fds_index.end()) [[unlikely]] {
    fds.at(fds_it->second).events = to_native_events(events);
    return false;
  }

  auto fds_to_insert_it =
      std::find_if(fds_to_insert.begin(), fds_to_insert.end(),
                   [&fd](const ::pollfd& pollfd) {
                     return pollfd.fd == fd.native_handle();
                   });
  if (fds_to_insert_it != fds_to_insert.end()) [[unlikely]] {
    fds_to_insert_it->events = to_native_events(events);
    return false;
  }

  ::pollfd pollfd{
      .fd = fd.native_handle(),
      .events = to_native_events(events),
      .revents = kEmptyEvents,
  };

  if (fds.size() < fds.capacity()) [[likely]] {
    fds.push_back(std::move(pollfd));
  } else {
    fds_to_insert.push_back(std::move(pollfd));
  }
  return true;
}

bool poller::erase(const io::fd& fd) {
  auto& [fds, fds_index, fds_to_insert] = *pimpl_;

  auto it = fds_index.find(fd.native_handle());
  if (it != fds_index.end()) [[likely]] {
    fds.at(it->second).fd = -fds.at(it->second).fd;
    return true;
  }

  auto fds_to_insert_it =
      std::find_if(fds_to_insert.begin(), fds_to_insert.end(),
                   [&fd](const ::pollfd& pollfd) {
                     return pollfd.fd == fd.native_handle();
                   });
  if (fds_to_insert_it != fds_to_insert.end()) [[unlikely]] {
    fds_to_insert_it->fd = -fds_to_insert_it->fd;
    return true;
  }

  return false;
}

std::size_t poller::try_poll(std::chrono::milliseconds timeout) {
  auto& [fds, fds_index, fds_to_insert] = *pimpl_;

  if (!fds_to_insert.empty()) [[unlikely]] {
    fds.reserve(fds.size() + fds_to_insert.size());
    for (std::size_t i = 0; i < fds_to_insert.size(); ++i) {
      fds_index.emplace(fds_to_insert.at(i).fd, fds.size() + i);
    }
    fds.insert(fds.end(), fds_to_insert.begin(), fds_to_insert.end());
  }

  int affected = ::poll(fds.data(), fds.size(), timeout.count());
  if (affected == kSyscallError) [[unlikely]] {
    throw std::runtime_error(
        std::format("poll() failed: {}", std::strerror(errno)));
  }

  std::size_t unhandled = 0;
  for (std::size_t i = 0; i < fds.size() && affected > 0; ++i) {
    if (fds.at(i).fd < 0) [[unlikely]] {
      fds_index.at(fds.back().fd) = fds_index.at(-fds.at(i).fd);
      std::swap(fds.back(), fds.at(i));

      fds_index.erase(fds.at(i).fd);
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
        callback_(*std::launder(reinterpret_cast<const io::fd*>(&fds.at(i).fd)),
                  from_native_events(fds.at(i).revents));
      } catch (...) {
        unhandled += 1;
      }
    }
    affected -= 1;
  }
  return unhandled;
}

std::size_t poller::poll() { return try_poll(kBlockingTimeout); }

}  // namespace core::io
