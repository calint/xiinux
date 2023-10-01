// reviewed: 2023-09-27
#pragma once
#include "xprinter.hpp"
#include <cstdio>
#include <string.h>
#include <sys/types.h>

namespace xiinux {
template <unsigned N = 1024> class strb final : public xprinter {
  size_t len_ = 0;
  char buf_[N];

public:
  inline strb() {}
  inline strb(const char *str) { p(str); }
  inline strb(const char *str, const size_t str_len) { p(str, str_len); }
  inline const char *buf() const { return buf_; }
  inline size_t len() const { return len_; }

  inline strb &rst() {
    len_ = 0;
    return *this;
  }

  inline strb &p(const std::string_view sv) override {
    const char *str = sv.data();
    const size_t str_len = sv.size();
    const ssize_t rem =
        ssize_t(sizeof(buf_)) - ssize_t(len_) - ssize_t(str_len);
    if (rem < 0)
      throw "strb:1:buffer full";
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

  inline strb &p(const int i) override {
    char str[32];
    const int len = snprintf(str, sizeof(str), "%d", i);
    if (len < 0 or size_t(len) >= sizeof(str))
      throw "strb:2";
    return p({str, size_t(len)});
  }

  inline strb &p(const size_t sz) override {
    char str[32];
    const int len = snprintf(str, sizeof(str), "%zu", sz);
    if (len < 0 or size_t(len) >= sizeof(str))
      throw "strb:3";
    return p({str, size_t(len)});
  }

  inline strb &p_ptr(const void *ptr) override {
    char str[32];
    const int len = snprintf(str, sizeof(str), "%p", ptr);
    if (len < 0 or size_t(len) >= sizeof(str))
      throw "strb:4";
    return p({str, size_t(len)});
  }

  inline strb &p_hex(const int i) override {
    char str[32];
    const int len = snprintf(str, sizeof(str), "%x", i);
    if (len < 0 or size_t(len) >= sizeof(str))
      throw "strb:5";
    return p({str, size_t(len)});
  }

  inline strb &p(const char ch) override {
    if (sizeof(buf_) - len_ == 0)
      throw "strb:6";
    *(buf_ + len_) = ch;
    len_++;
    return *this;
  }

  inline strb &nl() override { return p('\n'); }

  template <unsigned M> inline strb &p(const strb<M> &sb) {
    p({sb.buf(), sb.len()});
    return *this;
  }

  std::string_view string_view() const { return {buf_, len_}; }
};
} // namespace xiinux
