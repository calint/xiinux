#pragma once
#include "conf.hpp"
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
  char buf_[conf::chunky_buf_size_in_bytes]; //? uninitialized
  bool finished_ = false;
  int sockfd_;

public:
  inline chunky(int sockfd) : sockfd_{sockfd} {}
  inline ~chunky() override {
    if (!finished_) {
      finish();
    }
  }

  inline chunky &flush() {
    if (len_ == 0)
      return *this;

    // send header
    char buf[32];
    const int len = snprintf(buf, sizeof(buf), "%lx\r\n", len_);
    if (len < 0 or size_t(len) >= sizeof(buf))
      throw "1";
    io_send(buf, size_t(len), true, true);

    size_t sent_total = 0;
    while (true) {
      while (true) {
        const size_t nsend = len_ - sent_total;
        const size_t nsent = io_send(buf_ + sent_total, nsend, false, true);
        sent_total += nsent;
        if (nsent == nsend)
          break;
        //?? blocking
        perr("would block, try again");
      }
      if (sent_total == len_)
        break;
    }
    io_send("\r\n", 2, true, true); // 2 is string length
    len_ = 0;
    return *this;
  }

  inline chunky &finish() {
    flush();
    constexpr char fin[] = "0\r\n\r\n";
    // -1 to exclude terminator '\0'
    io_send(fin, sizeof(fin) - 1, true, false);
    finished_ = true;
    return *this;
  }

  // sends current buffer as is
  inline chunky &send_response_header() {
    io_send(buf_, len_, true, true);
    len_ = 0;
    return *this;
  }

  inline chunky &p(/*scans*/ const char *str) override {
    return p(str, strlen(str));
  }

  inline chunky &p(/*scans*/ const char *str, const size_t strlen) override {
    const ssize_t sizeofbuf = sizeof(buf_);
    const ssize_t bufrem = sizeofbuf - ssize_t(len_);
    ssize_t rem = bufrem - ssize_t(strlen);
    if (rem >= 0) { // str fits in remaining buffer
      strncpy(buf_ + len_, str, strlen);
      len_ += strlen;
      return *this;
    }
    // str does not fit in buffer
    strncpy(buf_ + len_, str, size_t(bufrem)); // fill buffer
    len_ += size_t(bufrem);
    flush();
    const char *s = str + bufrem; // pointer to remaining part of 'str'
    rem = -rem;                   // remaining chars to be sent from 'str'
    while (true) {
      const ssize_t n = sizeofbuf - ssize_t(len_);
      const ssize_t m = rem <= n ? rem : n;
      strncpy(buf_ + len_, s, size_t(m));
      len_ += size_t(m);
      flush();
      rem -= ssize_t(m);
      if (rem == 0)
        return *this;
      s += m;
    }
  }

  inline chunky &p(const int i) override {
    char str[32];
    const int n = snprintf(str, sizeof(str), "%d", i);
    if (n < 0 or size_t(n) >= sizeof(str))
      throw "chunky:1";
    return p(str, size_t(n));
  }

  inline chunky &p(const size_t i) override {
    char str[32];
    const int n = snprintf(str, sizeof(str), "%zd", i);
    if (n < 0 or size_t(n) >= sizeof(str))
      throw "chunky:2";
    return p(str, size_t(n));
  }

  inline chunky &p_ptr(const void *ptr) override {
    char str[32];
    const int n = snprintf(str, sizeof(str), "%p", ptr);
    if (n < 0 or size_t(n) >= sizeof(str))
      throw "chunky:3";
    return p(str, size_t(n));
  }

  inline chunky &p_hex(const unsigned i) override {
    char str[32];
    const int n = snprintf(str, sizeof(str), "%ux", i);
    if (n < 0 or size_t(n) >= sizeof(str))
      throw "chunky:4";
    return p(str, size_t(n));
  }

  inline chunky &p(const char ch) override {
    if (sizeof(buf_) - len_ == 0)
      flush();
    *(buf_ + len_++) = ch;
    return *this;
  }

  inline chunky &nl() override { return p('\n'); }

  // html5
  inline chunky &html5(const char *title = "") override {
    const char s[] = "<!doctype html><script src=/x.js></script><link "
                     "rel=stylesheet href=/x.css>";
    return p(s, sizeof(s))
        .p("<title>", sizeof("<title>"))
        .p(title)
        .p("</title>", sizeof("</title>"));
  }
  inline chunky &to(FILE *f) {
    char fmt[32];
    const int n = snprintf(fmt, sizeof(fmt), "%%%zus", len_);
    if (n < 0 or size_t(n) >= sizeof(fmt))
      throw "chunky:err1";
    fprintf(f, fmt, buf_);
    return *this;
  }

private:
  inline size_t io_send(const char *buf, size_t len,
                        bool throw_if_send_not_complete = false,
                        const bool buffer_send = false) {
    stats.writes++;
    const int flags = buffer_send ? MSG_NOSIGNAL | MSG_MORE : MSG_NOSIGNAL;
    const ssize_t n = send(sockfd_, buf, len, flags);
    if (n == -1) {
      if (errno == EPIPE or errno == ECONNRESET)
        throw signal_connection_lost;
      stats.errors++;
      throw "chunky:io_send";
    }
    stats.output += size_t(n);
    if (conf::print_traffic) {
      const ssize_t m = write(conf::print_traffic_fd, buf_, size_t(n));
      if (m == -1 or m != n) {
        perror("writing traffic");
      }
    }
    if (throw_if_send_not_complete and size_t(n) != len) {
      stats.errors++;
      throw "send not complete";
    }
    return size_t(n);
  }
};
} // namespace xiinux
