#pragma once
#include "xprinter.hpp"
#include <cstdio>
#include <string.h>
#include <sys/types.h>

namespace xiinux {
template <unsigned N = 4096> class strb final : public xprinter {
  size_t len_ = 0;
  char buf_[N];

public:
  inline strb() {}
  inline strb(const char *str) { p(str); }
  // inline strb &flush() override { return *this; }
  inline const char *buf() const { return buf_; }
  inline size_t size() const { return len_; }

  inline strb &rst() {
    len_ = 0;
    return *this;
  }

  inline strb &p(/*copies*/ const char *str) override {
    const size_t strlen = strnlen(str, sizeof(buf_));
    return p(str, strlen);
  }

  inline strb &p(/*copies*/ const char *str, const size_t strlen) override {
    const ssize_t rem = ssize_t(sizeof(buf_)) - ssize_t(len_) - ssize_t(strlen);
    if (rem < 0)
      throw "strb:1:buffer full";
    strncpy(buf_ + len_, str, strlen);
    len_ += strlen;
    return *this;
  }

  inline strb &p(const int i) override {
    char str[32];
    const int len = snprintf(str, sizeof(str), "%d", i);
    if (len < 0 or size_t(len) >= sizeof(str))
      throw "strb:2";
    return p(str, size_t(len));
  }

  inline strb &p(const size_t i) override {
    char str[32];
    const int len = snprintf(str, sizeof(str), "%zu", i);
    if (len < 0 or size_t(len) >= sizeof(str))
      throw "strb:3";
    return p(str, size_t(len));
  }

  inline strb &p_ptr(const void *ptr) override {
    char str[32];
    const int len = snprintf(str, sizeof(str), "%p", ptr);
    if (len < 0 or size_t(len) >= sizeof(str))
      throw "strb:4";
    return p(str, size_t(len));
  }

  inline strb &p_hex(const unsigned i) override {
    char str[32];
    const int len = snprintf(str, sizeof(str), "%ux", i);
    if (len < 0 or size_t(len) >= sizeof(str))
      throw "strb:5";
    return p(str, size_t(len));
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
    p(sb.buf(), sb.size());
    return *this;
  }

  // html5
  inline strb &html5(const char *title = "") override {
    constexpr char s[] = "<!doctype html><script src=/x.js></script><link "
                         "rel=stylesheet href=/x.css>";
    // -1 to not copy the terminator \0
    // 7 and 8 are the number of characters to copy
    return p(s, sizeof(s) - 1).p("<title>", 7).p(title).p("</title>", 8);
  }

  inline strb &to(FILE *f) {
    char fmt[32];
    const int res = snprintf(fmt, sizeof(fmt), "%%%zus", len_);
    if (res < 0 or res >= sizeof(fmt))
      throw "strb:8";
    fprintf(f, fmt, buf_);
    return *this;
  }
};
} // namespace xiinux
