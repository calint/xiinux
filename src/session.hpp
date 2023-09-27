#pragma once
#include "widget.hpp"
namespace xiinux {
class session final {
  const char *key_;
  lut<char *> kvp_;
  lut<widget *> widgets_;

public:
  inline session(/*take*/ char *session_id) : key_{session_id} {
    stats.sessions++;
  }

  inline ~session() {
    stats.sessions--;
    delete[] key_;
    kvp_.delete_content(true);
    widgets_.delete_content(true);
  }

  inline const char *id() const { return key_; }
  inline void *operator[](const char *key) { return kvp_[key]; }
  inline widget *get_widget(const char *key) { return widgets_[key]; }

  inline void put(/*take*/ char *key, /*take*/ char *data) {
    kvp_.put(key, data);
  }

  inline void put_widget(/*take*/ char *key, /*take*/ widget *o) {
    widgets_.put(key, o);
  }
};
} // namespace xiinux
