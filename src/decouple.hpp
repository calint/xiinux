// reviewed: 2023-09-27
#pragma once
#include "stats.hpp"
#include "defines.hpp"
#include "conf.hpp"
#include <cerrno>
#include <sys/socket.h>
#include <unistd.h>

// shared by server and sock to avoid circular reference
// 'sock' uses 'epoll_fd', 'homepage' and 'widget_new'
// 'sock' does not refer to 'server'
// 'server' uses 'epoll_fd' and 'homepage'. refers to 'sock'
// 'io_send(...)' used in 'sock', 'chunky' and 'reply'
// 'widget_new(...) used in 'sock'

namespace xiinux {
class doc;
class widget;

static int epoll_fd;
static doc *homepage;

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
    throw "sock:io_send";
  }
  const size_t nbytes_sent = size_t(n);
  stats.output += nbytes_sent;

  if (conf::print_traffic) {
    const ssize_t m = write(conf::print_traffic_fd, buf, nbytes_sent);
    if (m == -1 or m != n) {
      perror("reply:io_send2");
    }
  }

  if (throw_if_send_not_complete and nbytes_sent != buf_len) {
    stats.errors++;
    throw "sock:sendnotcomplete";
  }

  return nbytes_sent;
}
} // namespace xiinux

namespace xiinux::web {
static /*give*/ widget *widget_new(const char *qs);
}
