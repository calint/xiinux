// reviewed: 2023-09-27
#pragma once
#include "exceptions.hpp"
#include "stats.hpp"
#include <arpa/inet.h>
#include <array>
#include <chrono>
#include <memory>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unordered_map>

// solves circular references
// shared by 'server', 'sock', 'reply' and 'chunky'
// 'sock' uses 'epoll_fd', 'homepage' and 'widget_new'
// 'server' uses 'epoll_fd' and 'homepage'. refers to 'sock'
// 'io_send(...)' used by 'sock', 'reply' and 'chunky'
// 'widget_factory_for_path(...) used by 'sock'

namespace xiinux {

class widget;

using map_headers = std::unordered_map<std::string_view, std::string_view>;
using map_session = std::unordered_map<std::string, std::string>;
using map_widgets = std::unordered_map<std::string, std::unique_ptr<widget>>;
using widget_factory_func_ptr = widget *(*)();

static int epoll_fd{};

static inline auto
io_send(const int fd, const void *buf, size_t buf_len,
        const bool buffer_send = false,
        const bool throw_if_send_not_complete = true) -> size_t {
  stats.writes++;
  const int flags = buffer_send ? MSG_NOSIGNAL | MSG_MORE : MSG_NOSIGNAL;
  const ssize_t n = send(fd, buf, buf_len, flags);
  if (n == -1) {
    if (errno == EPIPE or errno == ECONNRESET) {
      throw client_closed_exception{};
    }
    if (throw_if_send_not_complete) {
      throw client_exception{"io_send:1"};
    }
    return 0;
  }
  const auto nbytes_sent = size_t(n);
  stats.output += nbytes_sent;

  if (conf::print_traffic) {
    const ssize_t m = write(conf::print_traffic_fd, buf, nbytes_sent);
    if (m == -1 or m != n) {
      perror("io_send:2");
    }
  }

  if (throw_if_send_not_complete and nbytes_sent != buf_len) {
    throw client_exception{"io_send:3"};
  }

  return nbytes_sent;
}

static inline auto
io_send(const int fd, const std::string_view &sv,
        const bool buffer_send = false,
        const bool throw_if_send_not_complete = true) -> size_t {
  return io_send(fd, sv.data(), sv.size(), buffer_send,
                 throw_if_send_not_complete);
}

inline static auto
current_time_to_str(std::array<char, 26> &time_str_buf) -> const char * {
  const auto chrono_now = std::chrono::system_clock::now();
  const auto time_now = std::chrono::system_clock::to_time_t(chrono_now);
  if (!std::strftime(time_str_buf.data(), time_str_buf.size(), "%F %T",
                     std::localtime(&time_now))) {
    // some error
    time_str_buf[0] = '\0';
  }
  return time_str_buf.data();
}

inline static auto ip_addr_to_str(std::array<char, INET_ADDRSTRLEN> &dst,
                                  void *s_addr) -> const char * {
  if (!inet_ntop(AF_INET, s_addr, dst.data(), unsigned(dst.size()))) {
    perror("ip address to text");
    dst[0] = 0;
  }
  return dst.data();
}

} // namespace xiinux

namespace xiinux::web {

static inline void widget_init_path_to_factory_map();

static inline auto widget_factory_for_path(const std::string_view &path)
    -> widget_factory_func_ptr;

} // namespace xiinux::web
