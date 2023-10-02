// reviewed: 2023-09-27
#pragma once
#include "xprinter.hpp"
#include <cstdio>
#include <string.h>
#include <sys/types.h>

namespace xiinux {
template <unsigned N = 1024> class strb final : public xprinter {
  char buf_[N]{};
  size_t len_ = 0;

public:
  inline auto buf() const -> const char * { return buf_; }
  inline auto len() const -> size_t { return len_; }

  inline auto rst() -> strb & {
    len_ = 0;
    return *this;
  }

  inline auto p(const std::string_view sv) -> strb & override {
    const char *str = sv.data();
    const size_t str_len = sv.size();
    const ssize_t rem =
        ssize_t(sizeof(buf_)) - ssize_t(len_) - ssize_t(str_len);
    if (rem < 0)
      throw client_exception{"strb:1:buffer full"};
    strncpy(buf_ + len_, str, str_len);
    len_ += str_len;
    return *this;
  }

  // inline strb &p(/*copies*/ const char *str) override {
  //   if (!str)
  //     return *this;
  //   const size_t str_len = strnlen(str, sizeof(buf_));
  //   // note. next statement will throw if buffer is overrun
  //   return p(str, str_len);
  // }

  // inline strb &p(/*copies*/ const char *str, const size_t str_len) override {
  //   const ssize_t rem =
  //       ssize_t(sizeof(buf_)) - ssize_t(len_) - ssize_t(str_len);
  //   if (rem < 0)
  //     throw "strb:1:buffer full";
  //   strncpy(buf_ + len_, str, str_len);
  //   len_ += str_len;
  //   return *this;
  // }

  inline auto p(const int i) -> strb & override {
    char str[32];
    const int len = snprintf(str, sizeof(str), "%d", i);
    if (len < 0 or size_t(len) >= sizeof(str))
      throw client_exception{"strb:2"};
    return p({str, size_t(len)});
  }

  inline auto p(const size_t sz) -> strb & override {
    char str[32];
    const int len = snprintf(str, sizeof(str), "%zu", sz);
    if (len < 0 or size_t(len) >= sizeof(str))
      throw client_exception{"strb:3"};
    return p({str, size_t(len)});
  }

  inline auto p_ptr(const void *ptr) -> strb & override {
    char str[32];
    const int len = snprintf(str, sizeof(str), "%p", ptr);
    if (len < 0 or size_t(len) >= sizeof(str))
      throw client_exception{"strb:4"};
    return p({str, size_t(len)});
  }

  inline auto p_hex(const int i) -> strb & override {
    char str[32];
    const int len = snprintf(str, sizeof(str), "%x", i);
    if (len < 0 or size_t(len) >= sizeof(str))
      throw client_exception{"strb:5"};
    return p({str, size_t(len)});
  }

  inline auto p(const char ch) -> strb & override {
    if (sizeof(buf_) - len_ == 0)
      throw client_exception{"strb:6"};
    *(buf_ + len_) = ch;
    len_++;
    return *this;
  }

  inline auto nl() -> strb & override { return p('\n'); }

  template <unsigned M> inline auto p(const strb<M> &sb) -> strb & {
    p({sb.buf(), sb.len()});
    return *this;
  }

  inline auto string_view() const -> std::string_view { return {buf_, len_}; }
};
} // namespace xiinux
