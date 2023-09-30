// reviewed: 2023-09-27
#pragma once
#include "lut.hpp"
#include "session.hpp"

namespace xiinux {
class sessions final {
  lut<session *> all_{K};

public:
  inline ~sessions() { all_.delete_content(false); }
  inline session *get(const char *sid) const { return all_[sid]; }

  inline void put(/*take*/ session *s, bool allow_overwrite = true) {
    all_.put(s->get_id(), s, allow_overwrite);
  }
} static sessions;
} // namespace xiinux
