#pragma once
#include "reply.hpp"

namespace xiinux {
class widget {
public:
  virtual ~widget() {}
  virtual void to(reply &x) = 0;

  // called when client is sending content
  // first call is with buf=nullptr, buf_len=0, content_len
  // subsequent calls are the portions of the content as read from the client
  virtual void on_content(reply &x, /*scan*/ const char *buf,
                          const size_t buf_len, const size_t received_len,
                          const size_t content_len) {}
};
} // namespace xiinux
