#pragma once
#include "reply.hpp"
namespace xiinux {
class widget {
public:
  virtual ~widget() {}
  virtual void to(reply &x) = 0;

  // called when client is sending content
  // first call is with buf=nullptr, len=0 and the length of the content in
  // total_content_len
  // subsequent calls are the portions of the content as read from the client
  virtual void on_content(reply &x, /*scan*/ const char *buf,
                          const size_t buflen, const size_t total_content_len) {
  }
};
} // namespace xiinux
