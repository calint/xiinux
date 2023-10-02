// reviewed: 2023-09-27
#pragma once
#include "reply.hpp"

namespace xiinux {

class widget {
public:
  virtual ~widget() = default;
  virtual void to(reply &x) = 0;

  // called when client is sending content
  // first call is with buf=nullptr, buf_len=0, received_len=0 and content_len
  // subsequent calls are the portions of the content as read from the client
  // received_len keeps track of received content this far
  virtual void on_content(reply &x, /*scan*/ const char *buf,
                          const size_t buf_len, const size_t received_len,
                          const size_t content_len) {}
};
} // namespace xiinux
