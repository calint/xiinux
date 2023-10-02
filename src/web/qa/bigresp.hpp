// reviewed: 2023-09-27
#pragma once
#include "../../strb.hpp"

namespace xiinux::web::qa {
class bigresp final : public widget {
  void to(reply &x) override {
    constexpr size_t buf_len = K * K;
    strb<buf_len> sb{};
    int i = 0;
    while (sb.len() + 128 < buf_len) {
      sb.p(i++).p(' ');
    }
    x.http(200, sb.string_view());
  }
};
} // namespace xiinux::web::qa
