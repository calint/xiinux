// reviewed: 2023-09-27
#pragma once
#include "reply.hpp"

namespace xiinux {

class widget {
public:
  widget() = default;
  widget(const widget &) = delete;
  auto operator=(const widget &) -> widget & = delete;
  widget(widget &&) = delete;
  auto operator=(widget &&) -> widget & = delete;

  virtual ~widget() = default;

  virtual void to(reply &x) = 0;

  // called when client is sending content
  // first call is with 'buf'=nullptr, 'buf_len'=0, 'received_len'=0 and
  // 'content_len'=total len. subsequent calls are the portions of the content
  // as read from the client 'received_len' keeps track of received content this
  // far
  virtual void on_content([[maybe_unused]] reply &x,
                          [[maybe_unused]] const char *buf,/*temporary*/
                          [[maybe_unused]] const size_t buf_len,
                          [[maybe_unused]] const size_t received_len,
                          [[maybe_unused]] const size_t content_len) {}
};

} // namespace xiinux
