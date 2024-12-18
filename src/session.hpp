// reviewed: 2023-09-27
#pragma once

namespace xiinux {

class session final {
  const std::string id_;
  map_session kvp_{};
  map_widgets widgets_{};

public:
  // clang-tidy recommends pass by value and move
  // cppcheck-suppress passedByValue
  inline explicit session(std::string id) noexcept : id_{std::move(id)} {
    stats.sessions++;
  }
  session(const session &) = delete;
  auto operator=(const session &) -> session & = delete;
  session(session &&) = delete;
  auto operator=(session &&) -> session & = delete;

  inline ~session() { stats.sessions--; }

  inline auto get_id() const -> const std::string & { return id_; }
  // inline auto operator[](const std::string &key) const -> const std::string &
  // {
  //   return kvp_.at(key);
  // }
  inline auto get_widget(const std::string_view &key) const -> widget * {
    auto it = widgets_.find(std::string{key}); //? creates a string
    if (it != widgets_.end()) {
      return it->second.get();
    }
    return nullptr;
  }
  inline auto get_lut() -> map_session & { return kvp_; }

  // inline void put(const std::string &key, std::string str) {
  //   kvp_[key] = std::move(str);
  // }

  inline void put_widget(std::string path, std::unique_ptr<widget> wgt);
};

} // namespace xiinux
