#pragma once
#include "xprinter.hpp"
#include <string.h>
namespace xiinux {
class strb final : public xprinter {
  size_t len_ = 0;
  char buf_[4096];

public:
  inline strb() {}
  inline strb(const char *str) { p(str); }
  inline strb &flush() override { return *this; }
  inline const char *buf() const { return buf_; }
  inline size_t size() const { return len_; }
  inline strb &rst() {
    len_ = 0;
    return *this;
  }
  inline strb &p(/*copies*/ const char *str) override {
    const size_t len = strnlen(str, sizeof(buf_) + 1); //? togetbufferoverrun
    const ssize_t rem = ssize_t(sizeof(buf_)) - ssize_t(len_) - ssize_t(len);
    if (rem < 0)
      throw "bufferoverrun";
    strncpy(buf_ + len_, str, len);
    len_ += len;
    return *this;
  }
  inline strb &p(const size_t len, /*copies*/ const char *str) override {
    const ssize_t rem = ssize_t(sizeof(buf_)) - ssize_t(len_) - ssize_t(len);
    if (rem < 0)
      throw "bufferoverrun";
    strncpy(buf_ + len_, str, len);
    len_ += len;
    return *this;
  }
  inline strb &p(const int i) override {
    char str[32];
    const int len = snprintf(str, sizeof(str), "%d", i);
    if (len < 0)
      throw "strb:1";
    return p(size_t(len), str);
  }
  inline strb &p(const size_t i) override {
    char str[32];
    const int len = snprintf(str, sizeof(str), "%zu", i);
    if (len < 0)
      throw "strb:2";
    return p(size_t(len), str);
  }
  inline strb &p_ptr(const void *ptr) override {
    char str[32];
    const int len = snprintf(str, sizeof(str), "%p", ptr);
    if (len < 0)
      throw "strb:3";
    return p(size_t(len), str);
  }
  inline strb &p_hex(const unsigned long i) override {
    char str[32];
    const int len = snprintf(str, sizeof(str), "%lx", i);
    if (len < 0)
      throw "strb:4";
    return p(size_t(len), str);
  }
  inline strb &p(char ch) override {
    if (sizeof(buf_) - len_ == 0)
      flush();
    *(buf_ + len_++) = ch;
    return *this;
  }
  inline strb &nl() override { return p('\n'); }
  inline strb &p(const strb &sb) {
    const ssize_t rem =
        ssize_t(sizeof(buf_)) - ssize_t(len_) - ssize_t(sb.len_);
    if (rem < 0)
      throw "bufferoverrun";
    strncpy(buf_ + len_, sb.buf_, sb.len_);
    len_ += sb.len_;
    return *this;
  }

  // html5
  inline strb &html5(const char *title = "") override {
    const char s[] = "<!doctype html><script src=/x.js></script><link "
                     "rel=stylesheet href=/x.css>";
    // -1 to not copy the terminator \0
    // 7 and 8 are the number of bytes to copy
    return p(sizeof(s) - 1, s).p(7, "<title>").p(title).p(8, "</title>");
  }
  inline strb &to(FILE *f) {
    char fmt[32];
    if (snprintf(fmt, sizeof(fmt), "%%%zus", len_) < 1)
      throw "strb:err1";
    fprintf(f, fmt, buf_);
    return *this;
  }
};
} // namespace xiinux
