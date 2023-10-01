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
    if (len_ == 0)
      return *this;

    send_chunk(buf_, len_);
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

  inline chunky &p(const std::string_view sv) override {
    return p(sv.data(), sv.size());
  }

  inline chunky &p(/*scans*/ const char *str) override {
    if (!str)
      return *this;
    return p(str, strlen(str)); //? strnlen
  }

  inline chunky &p(/*scans*/ const char *str, const size_t str_len) override {
    constexpr size_t buf_size = sizeof(buf_);
    // remaining space in buffer
    const size_t buf_rem = buf_size - len_;
    if (str_len <= buf_rem) {
      // str fits in buffer
      strncpy(buf_ + len_, str, str_len);
      len_ += str_len;
      if (len_ == buf_size) {
        flush();
      }
      return *this;
    }
    // str does not fit in buffer
    // if buffer is not empty then fill remaining buffer and flush
    size_t str_rem = str_len;
    if (len_ != 0) {
      strncpy(buf_ + len_, str, buf_rem);
      str += buf_rem; // pointer to remaining part of 'str'
      len_ += buf_rem;
      str_rem -= buf_rem;
      flush();
    }
    // loop as long as remaining str to send is bigger or equal to a full buffer
    while (str_rem >= buf_size) {
      // send a buffer sized chunk
      send_chunk(str, buf_size);
      str_rem -= buf_size;
      str += buf_size;
    }
    // if there is str left copy to buffer
    if (str_rem > 0) {
      // copy the remaining str into buffer
      strncpy(buf_, str, str_rem);
      len_ += size_t(str_rem);
    }
    return *this;
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

private:
  inline void send_chunk(const char *buf, const size_t buf_len) {
    // https://en.wikipedia.org/wiki/Chunked_transfer_encoding
    // chunk header
    char hdr[32];
    const int hdr_len = snprintf(hdr, sizeof(hdr), "%lx\r\n", buf_len);
    if (hdr_len < 0 or size_t(hdr_len) >= sizeof(hdr))
      throw "chunky:1";

    // send chunk header
    io_send(fd_, hdr, size_t(hdr_len), true);

    // send chunk
    size_t sent_total = 0;
    while (true) {
      while (true) {
        const size_t nsend = buf_len - sent_total;
        const size_t nsent = io_send(fd_, buf + sent_total, nsend, true, false);
        sent_total += nsent;
        if (nsent == nsend)
          break;
        //?? blocking
        printf("!!! would block: sent=%zu of %zu\n", nsent, nsend);
      }
      if (sent_total == buf_len)
        break;
    }
    // terminate the chunk
    io_send(fd_, "\r\n", 2, true); // 2 is string length
  }
};
} // namespace xiinux
