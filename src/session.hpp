// reviewed: 2023-09-27
#pragma once
#include "widget.hpp"

namespace xiinux {
class session final {
  const std::string id_;
  map_session kvp_{};
  lut<widget *, true, true> widgets_{};

public:
  inline session(std::string id) : id_{id} { stats.sessions++; }
  inline session(const session &) = delete;
  inline session &operator=(const session &) = delete;

  inline ~session() { stats.sessions--; }

  inline const std::string &get_id() const { return id_; }
  inline const std::string &operator[](const std::string &key) const {
    return kvp_.at(key);
  }
  inline widget *get_widget(const char *key) const { return widgets_[key]; }
  inline map_session &get_lut() { return kvp_; }

  inline void put(std::string key, std::string str) { kvp_[key] = str; }

  inline void put_widget(/*take*/ const char *key, /*take*/ widget *wgt) {
    widgets_.put(key, wgt);
  }
};
} // namespace xiinux
