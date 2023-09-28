#pragma once
#include "reply.hpp"

namespace xiinux {
class doc final {
  char *buf_;
  size_t len_;

public:
  inline doc(/*copies*/ const char *data) {
    const size_t maxlen = K * M;
    len_ = strnlen(data, maxlen);
    if (len_ == maxlen)
      throw "doc:1";
    buf_ = new char[len_];
    memcpy(buf_, data, len_);
  }
  inline ~doc() { delete[] buf_; }
  inline void to(reply &x) const { x.send(buf_, len_); }
};
} // namespace xiinux
