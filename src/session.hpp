// reviewed: 2023-09-27
#pragma once
#include "widget.hpp"

namespace xiinux {
class session final {
  const std::string id_;
  map_session kvp_{};
  map_widgets widgets_{};

public:
  inline explicit session(std::string id) : id_{std::move(id)} {
    stats.sessions++;
  }
  session(const session &) = delete;
  session &operator=(const session &) = delete;
  session(session &&) = delete;
  session &operator=(session &&) = delete;

  inline ~session() { stats.sessions--; }

  inline const std::string &get_id() const { return id_; }
  inline const std::string &operator[](const std::string &key) const {
    return kvp_.at(key);
  }
  inline widget *get_widget(std::string_view key) const {
    auto it = widgets_.find(std::string{key}); //? creates a string
    if (it != widgets_.end())
      return it->second.get();
    return nullptr;
  }
  inline map_session &get_lut() { return kvp_; }

  inline void put(const std::string &key, std::string str) {
    kvp_[key] = std::move(str);
  }

  inline void put_widget(std::string path, std::unique_ptr<widget> wgt) {
    widgets_[std::move(path)] = std::move(wgt);
  }
};
} // namespace xiinux
