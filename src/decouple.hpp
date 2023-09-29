// reviewed: 2023-09-27
#pragma once
#include "conf.hpp"
#include "defines.hpp"
#include "stats.hpp"
#include <cerrno>
#include <memory>
#include <sys/socket.h>
#include <unistd.h>

// solves circular references
// shared by 'server', 'sock', 'reply' and 'chunky'
// 'sock' uses 'epoll_fd', 'homepage' and 'widget_new'
// 'server' uses 'epoll_fd' and 'homepage'. refers to 'sock'
// 'io_send(...)' used by 'sock', 'reply' and 'chunky'
// 'widget_new(...) used by 'sock'

namespace xiinux {
class doc;
class widget;

static int epoll_fd;
static std::unique_ptr<doc> homepage;

static inline size_t io_send(const int fd, const char *buf, size_t buf_len,
                             const bool buffer_send = false,
                             const bool throw_if_send_not_complete = true) {
  stats.writes++;
  const int flags = buffer_send ? MSG_NOSIGNAL | MSG_MORE : MSG_NOSIGNAL;
  const ssize_t n = send(fd, buf, buf_len, flags);
  if (n == -1) {
    if (errno == EPIPE or errno == ECONNRESET)
      throw signal_connection_lost;
    stats.errors++;
    throw "io_send:1";
  }
  const size_t nbytes_sent = size_t(n);
  stats.output += nbytes_sent;

  if (conf::print_traffic) {
    const ssize_t m = write(conf::print_traffic_fd, buf, nbytes_sent);
    if (m == -1 or m != n) {
      perror("io_send:2");
    }
  }

  if (throw_if_send_not_complete and nbytes_sent != buf_len) {
    stats.errors++;
    throw "io_send:3";
  }

  return nbytes_sent;
}
} // namespace xiinux

namespace xiinux::web {

static inline void widget_init_path_to_factory_map();
static inline widget *(*widget_factory_for_path(const char *path))();

} // namespace xiinux::web
