#pragma once
#include "reply.hpp"
namespace xiinux {
class widget {
public:
  virtual ~widget() {}
  virtual void to(reply &x) = 0;

  // first call is with buf=nullptr, len=0 and total_content_len the expected
  // length of the content subsequent calls are the portions of the content as
  // read from the client
  virtual void on_content(reply &x, /*scan*/ const char *buf, const size_t len,
                          const size_t total_content_len) {}
};
} // namespace xiinux
