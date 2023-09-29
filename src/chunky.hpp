// reviewed: 2023-09-27
#pragma once
#include "conf.hpp"
#include "decouple.hpp"
#include "defines.hpp"
#include "stats.hpp"
#include "strb.hpp"
#include "xprinter.hpp"
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>

namespace xiinux {
class chunky final : public xprinter {
  size_t len_ = 0;
  char buf_[conf::chunky_buf_size]; //? uninitialized
  bool finished_ = false;
  int fd_;

public:
  inline chunky(int sockfd) : fd_{sockfd} {}
  inline ~chunky() override {
    if (!finished_) {
      finish();
    }
  }

  inline chunky &flush() {
    // https://en.wikipedia.org/wiki/Chunked_transfer_encoding
    if (len_ == 0)
      return *this;

    // chunk header
    char hdr[32];
    const int hdr_len = snprintf(hdr, sizeof(hdr), "%lx\r\n", len_);
    if (hdr_len < 0 or size_t(hdr_len) >= sizeof(hdr))
      throw "chunky:1";

    // send chunk header
    io_send(fd_, hdr, size_t(hdr_len), true);

    // send chunk
    size_t sent_total = 0;
    while (true) {
      while (true) {
        const size_t nsend = len_ - sent_total;
        const size_t nsent =
            io_send(fd_, buf_ + sent_total, nsend, true, false);
        sent_total += nsent;
        if (nsent == nsend)
          break;
        //?? blocking
        perr("would block");
      }
      if (sent_total == len_)
        break;
    }
    // terminate the chunk
    io_send(fd_, "\r\n", 2, true); // 2 is string length
    len_ = 0;
    return *this;
  }

  inline chunky &finish() {
    if (finished_)
      throw "chunky:already finished";
    flush();
    constexpr char fin[] = "0\r\n\r\n";
    // -1 to exclude terminator '\0'
    io_send(fd_, fin, sizeof(fin) - 1);
    finished_ = true;
    return *this;
  }

  // sends current buffer as is
  inline chunky &send_response_header() {
    io_send(fd_, buf_, len_, true);
    len_ = 0;
    return *this;
  }

  inline chunky &p(/*scans*/ const char *str) override {
    return p(str, strlen(str)); //? strnlen
  }

  inline chunky &p(/*scans*/ const char *str, const size_t str_len) override {
    constexpr ssize_t buf_size = sizeof(buf_);
    const ssize_t buf_rem = buf_size - ssize_t(len_);
    ssize_t rem = buf_rem - ssize_t(str_len);
    if (rem >= 0) { // str fits in remaining buffer
      strncpy(buf_ + len_, str, str_len);
      len_ += str_len;
      return *this;
    }
    // str does not fit in buffer
    // fill remaining buffer and flush
    strncpy(buf_ + len_, str, size_t(buf_rem));
    len_ += size_t(buf_rem);
    flush();

    // loop until remaining str fits in buffer
    const char *s = str + buf_rem; // pointer to remaining part of 'str'
    rem = -rem;                    // remaining chars to be sent from 'str'
    while (true) {
      // does remaining str fit in buffer?
      const ssize_t m = rem <= buf_size ? rem : buf_size;
      strncpy(buf_, s, size_t(m)); // ? if m==buf_size do io_send skipping copy to buffer
      rem -= ssize_t(m);
      len_ += size_t(m);
      if (len_ == buf_size) { // if buffer full
        flush();
      }
      if (rem == 0)
        return *this;
      s += m;
    }
  }

  inline chunky &p(const int i) override {
    char str[32];
    const int n = snprintf(str, sizeof(str), "%d", i);
    if (n < 0 or size_t(n) >= sizeof(str))
      throw "chunky:2";
    return p(str, size_t(n));
  }

  inline chunky &p(const size_t sz) override {
    char str[32];
    const int n = snprintf(str, sizeof(str), "%zu", sz);
    if (n < 0 or size_t(n) >= sizeof(str))
      throw "chunky:3";
    return p(str, size_t(n));
  }

  inline chunky &p_ptr(const void *ptr) override {
    char str[32];
    const int n = snprintf(str, sizeof(str), "%p", ptr);
    if (n < 0 or size_t(n) >= sizeof(str))
      throw "chunky:4";
    return p(str, size_t(n));
  }

  inline chunky &p_hex(const int i) override {
    char str[32];
    const int n = snprintf(str, sizeof(str), "%x", i);
    if (n < 0 or size_t(n) >= sizeof(str))
      throw "chunky:5";
    return p(str, size_t(n));
  }

  inline chunky &p(const char ch) override {
    if (sizeof(buf_) - len_ == 0)
      flush();
    *(buf_ + len_++) = ch;
    return *this;
  }

  inline chunky &nl() override { return p('\n'); }
};
} // namespace xiinux
