// reviewed: 2023-09-27
#pragma once
#include "lut.hpp"
#include "session.hpp"

namespace xiinux {
class sessions final {
  lut<session *, false, true> all_{K};

public:
  inline session *get(const char *sid) const { return all_[sid]; }

  inline void put(/*take*/ session *s) { all_.put(s->get_id(), s, false); }
} static sessions;
} // namespace xiinux
