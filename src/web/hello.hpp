#pragma once
#include "../xiinux/widget.hpp"
namespace web {
using namespace xiinux;
class hello final : public widget {
  void to(reply &x) override { x.http2(200, "hello world\n"); }
};
} // namespace web
