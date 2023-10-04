// reviewed: 2023-09-27
//           2023-10-04
#pragma once
#include "xprinter.hpp"
#include <array>
#include <cstring>

namespace xiinux {
class chunky final : public xprinter {
  std::array<char, conf::chunky_buf_size> buf_{};
  size_t len_{};
  bool finished_{};
  int fd_{};

public:
  inline explicit chunky(const int file_descriptor) : fd_{file_descriptor} {}
  chunky(const chunky &) = delete;
  auto operator=(const chunky &) -> chunky & = delete;
  chunky(chunky &&) = delete;
  auto operator=(chunky &&) -> chunky & = delete;

  inline ~chunky() override {
    if (!finished_) {
      try {
        finish();
      } catch (...) {
        // ignore exception in case client is being forcefully closed
      }
    }
  }

  inline auto flush() -> chunky & {
    if (len_ == 0) {
      return *this;
    }
    send_chunk(buf_.data(), len_);
    len_ = 0;
    return *this;
  }

  inline auto finish() -> chunky & {
    if (finished_) {
      throw client_exception{"chunky:already finished"};
    }
    flush();
    io_send(fd_, "0\r\n\r\n"sv);
    finished_ = true;
    return *this;
  }

  // sends current buffer as is
  inline auto send_response_header() -> chunky & {
    io_send(fd_, buf_.data(), len_, true);
    len_ = 0;
    return *this;
  }

  // xwriter implementation

  inline auto p(const std::string_view &sv) -> chunky & override {
    const char *str = sv.data();
    const size_t str_len = sv.size();
    constexpr size_t buf_size = sizeof(buf_);
    // remaining space in buffer
    const size_t buf_rem = buf_size - len_;
    if (str_len <= buf_rem) {
      // str fits in buffer
      strncpy(buf_.data() + len_, str, str_len);
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
      strncpy(buf_.data() + len_, str, buf_rem);
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
      strncpy(buf_.data(), str, str_rem);
      len_ += size_t(str_rem);
    }
    return *this;
  }

  inline auto p(const int i) -> chunky & override {
    std::array<char, array_size_nums> str{};
    const int n = snprintf(str.data(), str.size(), "%d", i);
    if (n < 0 or size_t(n) >= str.size()) {
      throw client_exception{"chunky:2"};
    }
    return p({str.data(), size_t(n)});
  }

  inline auto p(const size_t sz) -> chunky & override {
    std::array<char, array_size_nums> str{};
    const int n = snprintf(str.data(), str.size(), "%zu", sz);
    if (n < 0 or size_t(n) >= str.size()) {
      throw client_exception{"chunky:3"};
    }
    return p({str.data(), size_t(n)});
  }

  inline auto p_ptr(const void *ptr) -> chunky & override {
    std::array<char, array_size_nums> str{};
    const int n = snprintf(str.data(), str.size(), "%p", ptr);
    if (n < 0 or size_t(n) >= str.size()) {
      throw client_exception{"chunky:4"};
    }
    return p({str.data(), size_t(n)});
  }

  inline auto p_hex(const int i) -> chunky & override {
    std::array<char, array_size_nums> str{};
    const int n = snprintf(str.data(), str.size(), "%x", i);
    if (n < 0 or size_t(n) >= str.size()) {
      throw client_exception{"chunky:5"};
    }
    return p({str.data(), size_t(n)});
  }

  inline auto p(const char ch) -> chunky & override {
    if (sizeof(buf_) - len_ == 0) {
      flush();
    }
    *(buf_.data() + len_++) = ch;
    return *this;
  }

  inline auto nl() -> chunky & override { return p('\n'); }

  // end of xprinter implementation

private:
  inline void send_chunk(const char *buf, const size_t buf_len) const {
    // https://en.wikipedia.org/wiki/Chunked_transfer_encoding
    // chunk header
    std::array<char, 32> hdr{};
    const int hdr_len = snprintf(hdr.data(), hdr.size(), "%lx\r\n", buf_len);
    if (hdr_len < 0 or size_t(hdr_len) >= hdr.size()) {
      throw client_exception{"chunky:1"};
    }
    // send chunk header
    io_send(fd_, hdr.data(), size_t(hdr_len), true);
    // send chunk
    size_t sent_total = 0;
    while (true) {
      while (true) {
        const size_t nsend = buf_len - sent_total;
        const size_t nsent = io_send(fd_, buf + sent_total, nsend, true, false);
        sent_total += nsent;
        if (nsent == nsend) {
          break;
        }
        //?? blocking
        printf("!!! would block: sent=%zu of %zu\n", nsent, nsend);
      }
      if (sent_total == buf_len) {
        break;
      }
    }
    // terminate the chunk
    io_send(fd_, "\r\n"sv, true); // 2 is string length
  }

  static constexpr size_t array_size_nums = 32;
};
} // namespace xiinux
