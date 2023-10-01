// reviewed: 2023-09-28
#pragma once
#include "reply.hpp"

namespace xiinux {
class doc final {
  std::string buf_{};

public:
  inline doc(const char *str, const size_t str_len) : buf_{str, str_len} {}
  inline void to(reply &x) const { x.send(buf_.data(), buf_.size()); }
};
} // namespace xiinux
