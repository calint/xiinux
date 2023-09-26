#pragma once
#include "../xiinux/widget.hpp"
namespace xiinux::web {
class hello final : public widget {
  void to(reply &x) override { x.http(200, "hello world\n"); }
};
} // namespace xiinux::web
