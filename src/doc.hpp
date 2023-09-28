// reviewed: 2023-09-28
#pragma once
#include "reply.hpp"

namespace xiinux {
class doc final {
  char *buf_ = nullptr;
  size_t len_ = 0;

public:
  inline doc(/*copies*/ const char *str, const size_t str_len) : len_{str_len} {
    buf_ = new char[len_];
    memcpy(buf_, str, len_);
  }
  inline doc(const doc &) = delete;
  inline doc &operator=(const doc &) = delete;
  inline ~doc() { delete[] buf_; }
  inline void to(reply &x) const { x.send(buf_, len_); }
};
} // namespace xiinux
