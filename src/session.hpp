// reviewed: 2023-09-27
#pragma once
#include "widget.hpp"

namespace xiinux {
class session final {
  const char *id_;
  lut<const char *> kvp_;
  lut<widget *> widgets_;

public:
  inline session(/*take*/ const char *id) : id_{id} { stats.sessions++; }

  inline ~session() {
    stats.sessions--;
    delete[] id_;
    kvp_.delete_content(true);
    widgets_.delete_content(true);
  }

  inline const char *id() const { return id_; }
  inline const char *operator[](const char *key) const { return kvp_[key]; }
  inline widget *get_widget(const char *key) const { return widgets_[key]; }

  inline void put(/*take*/ const char *key, /*take*/ const char *str) {
    kvp_.put(key, str);
  }

  inline void put_widget(/*take*/ const char *key, /*take*/ widget *wgt) {
    widgets_.put(key, wgt);
  }
};
} // namespace xiinux
