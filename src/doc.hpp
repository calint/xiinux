// reviewed: 2023-09-28
#pragma once
#include "reply.hpp"

namespace xiinux {
class doc final {
  std::string str_{};

public:
  inline doc(std::string str) : str_{str} {}
  inline void to(reply &x) const { x.send(str_); }
};
} // namespace xiinux
