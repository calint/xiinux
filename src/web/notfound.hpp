#pragma once
#include "../widget.hpp"

namespace xiinux::web {
class notfound : public widget {
  virtual void to(reply &x) override { x.http(404, "not found\n"); }
};
} // namespace xiinux::web
