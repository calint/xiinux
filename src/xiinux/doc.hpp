#pragma once
#include "reply.hpp"
namespace xiinux {
class doc final {
  size_t len_;
  char *buf_;

public:
  inline doc(const char *data) {
    const size_t maxlen = K * M;
    len_ = strnlen(data, maxlen);
    if (len_ == maxlen)
      throw "overrun";
    buf_ = new char[len_];
    memcpy(buf_, data, len_);
  }
  inline ~doc() { delete[] buf_; }
  inline const char *buffer() const { return buf_; }
  inline size_t size_in_bytes() const { return len_; }
  inline void to(reply &x) const { x.send(buf_, len_); }
};
} // namespace xiinux
