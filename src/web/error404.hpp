#pragma once
#include "../widget.hpp"

namespace xiinux::web {
class error404 : public widget {
  virtual void to(reply &x) override { x.http(404, "not found\n"); }
};
} // namespace xiinux::web
