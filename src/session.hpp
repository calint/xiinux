// reviewed: 2023-09-27
#pragma once
#include "widget.hpp"

namespace xiinux {
class session final {
  const char *id_ = nullptr;
  lut_cstr<true, true, true> kvp_{};
  lut<widget *, true, true> widgets_{};

public:
  inline session(/*take*/ const char *id) : id_{id} { stats.sessions++; }
  inline session(const session &) = delete;
  inline session &operator=(const session &) = delete;

  inline ~session() {
    printf("*** delete session %p\n", static_cast<void *>(this));
    stats.sessions--;
    delete[] id_;
  }

  inline const char *get_id() const { return id_; }
  inline const char *operator[](const char *key) const { return kvp_[key]; }
  inline widget *get_widget(const char *key) const { return widgets_[key]; }
  inline lut_cstr<true, true, true> *get_lut() { return &kvp_; }

  inline void put(/*take*/ const char *key, /*take*/ const char *str) {
    kvp_.put(key, str);
  }

  inline void put_widget(/*take*/ const char *key, /*take*/ widget *wgt) {
    widgets_.put(key, wgt);
  }
};
} // namespace xiinux
