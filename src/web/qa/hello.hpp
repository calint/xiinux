// reviewed: 2023-09-27
#pragma once
#include "../../widget.hpp"

namespace xiinux::web::qa {
class hello final : public widget {
  void to(reply &x) override {
    constexpr const char msg[] = "hello world";
    // -1 ignores the terminating '\0'
    x.http(200, msg, sizeof(msg) - 1);
  }
};
} // namespace xiinux::web::qa
