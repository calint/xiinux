// reviewed: 2023-09-28
#pragma once
#include "chunky.hpp"
#include "strb.hpp"

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
  inline reply(const int fd, const std::string_view &path,
               const std::string_view &query, const map_headers &req_headers,
               map_session *session)
      : fd_{fd}, path_{path}, query_{query},
        req_headers_{req_headers}, session_{session} {}

  [[nodiscard]] inline auto get_path() const -> const std::string_view & {
    return path_;
  }
  [[nodiscard]] inline auto get_query() const -> const std::string_view & {
    return query_;
  }
  [[nodiscard]] inline auto get_req_headers() const -> const map_headers & {
    return req_headers_;
  }
  [[nodiscard]] inline auto get_session() const -> map_session * {
    return session_;
  }

  [[nodiscard]] inline auto reply_chunky(
      const std::string_view &content_type = "text/html;charset=utf-8"sv,
      const int response_code = 200) -> std::unique_ptr<chunky> {

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

  inline void send_session_id_at_next_opportunity(const std::string_view &id) {
    set_session_id_ = id;
  }

  inline auto
  http(const int code, const char *buf, size_t buf_len,
       const std::string_view &content_type = "text/html;charset=utf-8"sv)
      -> reply & {

    strb<conf::sock_request_header_buf_size> sb{};
    sb.p("HTTP/1.1 "sv)
        .p(code)
        .p("\r\nContent-Length: "sv)
        .p(buf_len)
        .p("\r\nContent-Type: "sv)
        .p(content_type)
        .p("\r\n"sv);

    if (!set_session_id_.empty()) {
      sb.p("Set-Cookie: i="sv)
          .p(set_session_id_)
          .p(";path=/;expires=Thu, 31-Dec-2099 00:00:00 GMT;SameSite=Lax\r\n"sv);
      set_session_id_ = {};
    }

    sb.p("\r\n"sv); // note. no .eos() because sb.string_view() will include it

    io_send(fd_, sb.string_view(), buf != nullptr);
    if (buf != nullptr) {
      io_send(fd_, buf, buf_len);
    }

    return *this;
  }

  inline auto
  http(const int code, const std::string_view &content,
       const std::string_view &content_type = "text/html;charset=utf-8"sv)
      -> reply & {
    return http(code, content.data(), content.size(), content_type);
  }

  inline auto send(const char *buf, size_t buf_len,
                   const bool buffer_send = false,
                   bool throw_if_send_not_complete = true) const -> size_t {
    return io_send(fd_, buf, buf_len, buffer_send, throw_if_send_not_complete);
  }

  // NOLINTNEXTLINE(modernize-use-nodiscard) not when exception thrown instead
  inline auto send(const std::string_view &sv, const bool buffer_send = false,
                   bool throw_if_send_not_complete = true) const -> size_t {
    return io_send(fd_, sv.data(), sv.size(), buffer_send,
                   throw_if_send_not_complete);
  }
};
} // namespace xiinux
