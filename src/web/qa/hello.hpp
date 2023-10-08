// reviewed: 2023-09-27
#pragma once

namespace xiinux::web::qa {
class hello final : public widget {
  void to(reply &x) override { x.http(200, "hello world"); }
};
} // namespace xiinux::web::qa
