#pragma once
#include "chunky.hpp"
#include "conf.hpp"
#include "stats.hpp"
#include "strb.hpp"
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
namespace xiinux {
class reply final {
  int fd_ = 0;
  const char *set_session_id_cookie_ = nullptr;

public:
  inline reply(const int fd = 0) : fd_{fd} {}
  inline /*give*/ chunky *reply_chunky() { return new chunky(fd_); }
  inline size_t io_send(const void *buf, size_t len,
                        bool throw_if_send_not_complete = false) {
    stats.writes++;
    const ssize_t n = ::send(fd_, buf, len, MSG_NOSIGNAL);
    if (n < 0) {
      if (errno == EPIPE or errno == ECONNRESET)
        throw signal_connection_reset_by_peer;
      stats.errors++;
      throw "iosend";
    }
    stats.output += size_t(n);

    if (conf::print_traffic) {
      const ssize_t m = write(conf::print_traffic_fd, buf, size_t(n));
      if (m == -1 or m != n) {
        perror("reply:io_send");
      }
    }

    if (throw_if_send_not_complete and size_t(n) != len) {
      stats.errors++;
      throw "sendnotcomplete";
    }

    return size_t(n);
  }

  inline reply &send(const char *data, const size_t len) {
    io_send(data, len, true);
    return *this;
  }

  inline void send_session_id_at_next_opportunity(const char *id) {
    set_session_id_cookie_ = id;
  }

  inline reply &http(const int code, const char *content = nullptr,
                     size_t len = 0) {
    char header[256];
    if (content and len == 0) {
      len = strnlen(content, K * M);
    }
    int n = 0;
    if (set_session_id_cookie_) {
      n = snprintf(header, sizeof(header),
                   "HTTP/1.1 %d\r\nContent-Length: %zu\r\nSet-Cookie: "
                   "i=%s;path=/;expires=Thu, 31-Dec-2099 00:00:00 "
                   "GMT;SameSite=Lax\r\n\r\n",
                   code, len, set_session_id_cookie_);
      set_session_id_cookie_ = nullptr;
    } else {
      n = snprintf(header, sizeof(header),
                   "HTTP/1.1 %d\r\nContent-Length: %zu\r\n\r\n", code, len);
    }
    if (n < 0 or size_t(n) >= sizeof(header))
      throw "reply:http:headerbuf";

    io_send(header, size_t(n), true);
    if (content) {
      io_send(content, len, true);
    }
    return *this;
  }
  // inline reply &http(const int code, const strb<> &s) {
  //   return http(code, s.buf(), s.size());
  // }
  inline reply &http2(const int code, const char *str) {
    const size_t nn = strlen(str);
    return http(code, str, nn);
  }
};
} // namespace xiinux
