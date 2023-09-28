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
  const char *set_session_id_ = nullptr;

public:
  inline reply(const int fd = 0) : fd_{fd} {}
  inline /*give*/ chunky *
  reply_chunky(const char *content_type = "text/html; charset=utf-8",
               const int response_code = 200) {
    chunky *rsp = new chunky(fd_);
    // 9 and 2 are length of strings
    rsp->p("HTTP/1.1 ", 9).p(response_code).p("\r\n", 2);
    if (set_session_id_) {
      // 14 and 60 are length of strings
      rsp->p("Set-Cookie: i=", 14)
          .p(set_session_id_)
          .p(";path=/;expires=Thu, 31-Dec-2099 00:00:00 GMT;SameSite=Lax\r\n",
             60);
    }
    // 41 and 4 are length of strings
    rsp->p("Transfer-Encoding:chunked\r\nContent-Type: ", 41)
        .p(content_type)
        .p("\r\n\r\n", 4);
    rsp->send_response_header();
    return rsp;
  }

  inline void send_session_id_at_next_opportunity(const char *id) {
    set_session_id_ = id;
  }

  inline reply &http(const int code, const char *content = nullptr,
                     size_t len = 0,
                     const char *content_type = "text/html; charset=utf-8") {
    char header[256];
    if (content and len == 0) {
      len = strnlen(content, K * M);
    }
    int n = 0;
    if (set_session_id_) {
      n = snprintf(header, sizeof(header),
                   "HTTP/1.1 %d\r\nContent-Length: %zu\r\nSet-Cookie: "
                   "i=%s;path=/;expires=Thu, 31-Dec-2099 00:00:00 "
                   "GMT;SameSite=Lax\r\nContent-Type: %s\r\n\r\n",
                   code, len, set_session_id_, content_type);
      set_session_id_ = nullptr;
    } else {
      n = snprintf(
          header, sizeof(header),
          "HTTP/1.1 %d\r\nContent-Length: %zu\r\nContent-Type: %s\r\n\r\n",
          code, len, content_type);
    }
    if (n < 0 or size_t(n) >= sizeof(header))
      throw "reply:http:1";

    io_send(fd_, header, size_t(n), content ? true : false);
    if (content) {
      io_send(fd_, content, len);
    }
    return *this;
  }

  inline size_t send(const char *ptr, size_t len,
                     const bool buffer_send = false,
                     bool throw_if_send_not_complete = true) {
    return io_send(fd_, ptr, len, buffer_send, throw_if_send_not_complete);
  }
};
} // namespace xiinux
