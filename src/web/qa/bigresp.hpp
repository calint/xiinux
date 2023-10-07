// reviewed: 2023-09-27
#pragma once
#include "../../strb.hpp"

namespace xiinux::web::qa {
class bigresp final : public widget {
  void to(reply &x) override {
    constexpr size_t buf_len = K * K * K;
    strb<buf_len> sb{};
    int i = 0;
    while (sb.len()< buf_len) {
      sb.p(' ');
    }
    x.http(200, sb.string_view());
  }
};
} // namespace xiinux::web::qa
