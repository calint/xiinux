// reviewed: 2023-09-28
#pragma once
#include "reply.hpp"

namespace xiinux {
class doc final {
  std::string str_{};

public:
  inline explicit doc(std::string str) : str_{std::move(str)} {}
  inline void to(const reply &x) const { x.send(str_); }
};
} // namespace xiinux
