// reviewed: 2023-09-27
#pragma once
#include "../../widget.hpp"

namespace xiinux::web::qa {
class bigresp final : public widget {
  void to(reply &x) override {
    constexpr size_t buf_len = 1024 * 1024;
    strb<buf_len> sb{};
    int i = 0;
    while (sb.len() + 128 < buf_len) {
      sb.p(i++).p(' ');
    }
    x.http(200, sb.buf(), sb.len());
  }
};
} // namespace xiinux::web::qa
