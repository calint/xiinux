// reviewed: 2023-09-28
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

class sock;

class reply final {
  int fd_{};
  std::string_view path_;
  std::string_view query_;
  const map_headers &req_headers_;
  map_session *session_;
  std::string_view set_session_id_{};

public:
  inline reply(const int fd, std::string_view path, std::string_view query,
               const map_headers &req_headers, map_session *session)
      : fd_{fd}, path_{path}, query_{query},
        req_headers_{req_headers}, session_{session} {}

  inline std::string_view get_path() const { return path_; }
  inline std::string_view get_query() const { return query_; }
  inline const map_headers &get_req_headers() const { return req_headers_; }
  inline map_session *get_session() const { return session_; }

  [[nodiscard]] inline std::unique_ptr<chunky>
  reply_chunky(std::string_view content_type = "text/html;charset=utf-8"sv,
               const int response_code = 200) {

    auto rsp{std::make_unique<chunky>(fd_)};

    rsp->p("HTTP/1.1 "sv).p(response_code).p("\r\n"sv);
    if (!set_session_id_.empty()) {
      rsp->p("Set-Cookie: i="sv)
          .p(set_session_id_)
          .p(";path=/;expires=Thu, 31-Dec-2099 00:00:00 GMT;SameSite=Lax\r\n"sv);
    }
    rsp->p("Transfer-Encoding:chunked\r\nContent-Type: "sv)
        .p(content_type)
        .p("\r\n\r\n"sv)
        .send_response_header();

    return rsp;
  }

  inline void send_session_id_at_next_opportunity(std::string_view id) {
    set_session_id_ = id;
  }

  inline reply &
  http(const int code, const char *buf, size_t buf_len,
       std::string_view content_type = "text/html;charset=utf-8"sv) {
    char header[256];
    int n = 0;
    if (!set_session_id_.empty()) {
      n = snprintf(header, sizeof(header),
                   "HTTP/1.1 %d\r\nContent-Length: %zu\r\nSet-Cookie: "
                   "i=%s;path=/;expires=Thu, 31-Dec-2099 00:00:00 "
                   "GMT;SameSite=Lax\r\nContent-Type: %s\r\n\r\n",
                   code, buf_len, set_session_id_.data(), content_type.data());
      set_session_id_ = {};
    } else {
      n = snprintf(
          header, sizeof(header),
          "HTTP/1.1 %d\r\nContent-Length: %zu\r\nContent-Type: %s\r\n\r\n",
          code, buf_len, content_type.data());
    }
    if (n < 0 or size_t(n) >= sizeof(header))
      throw "reply:http:1";

    io_send(fd_, header, size_t(n), buf);
    if (buf) {
      io_send(fd_, buf, buf_len);
    }
    return *this;
  }

  inline reply &
  http(const int code, std::string_view content,
       std::string_view content_type = "text/html;charset=utf-8"sv) {
    return http(code, content.data(), content.size(), content_type);
  }

  inline size_t send(const char *buf, size_t buf_len,
                     const bool buffer_send = false,
                     bool throw_if_send_not_complete = true) const {
    return io_send(fd_, buf, buf_len, buffer_send, throw_if_send_not_complete);
  }

  inline size_t send(std::string_view sv, const bool buffer_send = false,
                     bool throw_if_send_not_complete = true) const {
    return io_send(fd_, sv.data(), sv.size(), buffer_send,
                   throw_if_send_not_complete);
  }
};
} // namespace xiinux
