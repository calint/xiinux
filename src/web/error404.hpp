// reviewed: 2023-09-27
#pragma once
#include "../widget.hpp"

namespace xiinux::web {
class error404 final : public widget {
  void to(reply &x) override { x.http(404, "not found\n"); }
};
} // namespace xiinux::web
