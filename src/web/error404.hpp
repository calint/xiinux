// reviewed: 2023-09-27
#pragma once

namespace xiinux::web {
class error404 final : public widget {
  void to(reply &x) override {
    // 10 is string length
    x.http(404, {"not found\n", 10});
  }
};
} // namespace xiinux::web
