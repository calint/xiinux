// reviewed: 2023-09-27
#pragma once
#include "xprinter.hpp"
#include <array>
#include <cstdio>
#include <string.h>
#include <sys/types.h>

namespace xiinux {
template <unsigned SIZE = 1024> class strb final : public xprinter {
  std::array<char, SIZE> buf_{};
  size_t len_ = 0;

public:
  [[nodiscard]] inline auto buf() const -> const char * { return buf_.data(); }
  [[nodiscard]] inline auto len() const -> size_t { return len_; }

  inline auto rst() -> strb & {
    len_ = 0;
    return *this;
  }

  template <unsigned N> inline auto p(const strb<N> &sb) -> strb & {
    p({sb.buf(), sb.len()});
    return *this;
  }

  [[nodiscard]] inline auto string_view() const -> std::string_view {
    return {buf_.data(), len_};
  }

  inline auto eos() -> strb & { return p('\0'); }

  // xwrite implementation

  inline auto p(const std::string_view sv) -> strb & override {
    const char *str = sv.data();
    const size_t str_len = sv.size();
    const ssize_t rem = ssize_t(buf_.size()) - ssize_t(len_) - ssize_t(str_len);
    if (rem < 0) {
      throw client_exception{"strb:1:buffer full"};
    }
    strncpy(buf_.data() + len_, str, str_len);
    len_ += str_len;
    return *this;
  }

  inline auto p(const int i) -> strb & override {
    std::array<char, array_size_nums> str{};
    const int len = snprintf(str.data(), str.size(), "%d", i);
    if (len < 0 or size_t(len) >= str.size()) {
      throw client_exception{"strb:2"};
    }
    return p({str.data(), size_t(len)});
  }

  inline auto p(const size_t sz) -> strb & override {
    std::array<char, array_size_nums> str{};
    const int len = snprintf(str.data(), str.size(), "%zu", sz);
    if (len < 0 or size_t(len) >= str.size()) {
      throw client_exception{"strb:3"};
    }
    return p({str.data(), size_t(len)});
  }

  inline auto p_ptr(const void *ptr) -> strb & override {
    std::array<char, array_size_nums> str{};
    const int len = snprintf(str.data(), str.size(), "%p", ptr);
    if (len < 0 or size_t(len) >= str.size()) {
      throw client_exception{"strb:4"};
    }
    return p({str.data(), size_t(len)});
  }

  inline auto p_hex(const int i) -> strb & override {
    std::array<char, array_size_nums> str{};
    const int len = snprintf(str.data(), str.size(), "%x", i);
    if (len < 0 or size_t(len) >= str.size()) {
      throw client_exception{"strb:5"};
    }
    return p({str.data(), size_t(len)});
  }

  inline auto p(const char ch) -> strb & override {
    if (sizeof(buf_) - len_ == 0) {
      throw client_exception{"strb:6"};
    }
    *(buf_.data() + len_) = ch;
    len_++;
    return *this;
  }

  inline auto nl() -> strb & override { return p('\n'); }

  // end of xprinter implementation

private:
  static constexpr size_t array_size_nums = 32;
};
} // namespace xiinux
