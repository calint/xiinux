#pragma once
#include "uiprinter.hpp"
#include <vector>

namespace xiinux {
class uielem {
  uielem *parent_{};
  std::string name_{};
  std::string value_{};

public:
  uielem(uielem *parent, std::string name)
      : parent_{parent}, name_{std::move(name)} {}
  uielem(const uielem &) = default;
  auto operator=(const uielem &) -> uielem & = default;
  uielem(uielem &&) = default;
  auto operator=(uielem &&) -> uielem & = default;
  virtual ~uielem() = default;

  [[nodiscard]] inline auto get_name() const -> const std::string & {
    return name_;
  }

  [[nodiscard]] inline auto get_value() const -> const std::string & {
    return value_;
  }

  inline void set_value(const std::string &value) { value_ = value; }

  [[nodiscard]] inline auto get_id() const -> std::string {
    std::vector<const uielem *> elems{};
    const uielem *el = this;
    while (el != nullptr) {
      elems.push_back(el);
      el = el->parent_;
    }
    std::string id{};
    // clang++ does not support std::views::reverse(elems)
    // NOLINTNEXTLINE(modernize-loop-convert)
    for (auto it = elems.rbegin(); it != elems.rend(); ++it) {
      id.append("-").append((*it)->get_name());
    }
    return id;
  }

  virtual void render(uiprinter &x) { x.p(value_); }

  virtual auto get_child([[maybe_unused]] const std::string &name) -> uielem * {
    throw client_exception("elem:get_child");
  }

  virtual void on_callback([[maybe_unused]] uiprinter &x,
                           [[maybe_unused]] const std::string &name,
                           [[maybe_unused]] const std::string &param) {
    throw client_exception("elem:on_callback: no implementation");
  }
};
} // namespace xiinux
