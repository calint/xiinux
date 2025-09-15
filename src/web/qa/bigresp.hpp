// reviewed: 2023-09-27
#pragma once
#include "../../strb.hpp"
#include "../../widget.hpp"

namespace xiinux::web::qa {
class bigresp final : public widget {
  public:
    auto to(reply& x) -> void override {
        constexpr size_t buf_len = K * K * K;
        strb<buf_len> sb{};
        while (sb.len() < buf_len) {
            sb.p(' ');
        }
        x.http(200, sb.string_view());
    }
};
} // namespace xiinux::web::qa
