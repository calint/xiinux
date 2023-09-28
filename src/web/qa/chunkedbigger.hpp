// reviewed: 2023-09-27
#pragma once
#include "../../widget.hpp"
#include "../../chunky.hpp"
#include <memory>

// used in qa/test-coverage.sh
namespace xiinux::web::qa {
class chunkedbigger final : public widget {
public:
  void to(reply &r) override {
    std::unique_ptr<chunky> y(r.reply_chunky("text/plain;charset=utf-8"));
    xprinter &x = *y;

    constexpr size_t buf_len = 1024 * 1024;
    char buf[buf_len];
    char *p = buf;
    unsigned char ch = 0;
    for (unsigned i = 0; i < buf_len; i++) {
      *p = 'a' + char(ch % 26);
      p++;
      ch++;
    }
    x.p(buf, buf_len);
  }
};
} // namespace xiinux::web::qa
