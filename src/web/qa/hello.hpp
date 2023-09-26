#pragma once
#include "../../widget.hpp"
namespace xiinux::web::qa {
class hello final : public widget {
  void to(reply &x) override { x.http(200, "hello world\n"); }
};
} // namespace xiinux::web
