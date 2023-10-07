#pragma once
#include "uiprinter.hpp"
#include <vector>

namespace xiinux {
class uielem {
  uielem *parent_{};
  std::string name_{};
  std::string value_{};

public:
  inline uielem(uielem *parent, std::string name)
      : parent_{parent}, name_{std::move(name)} {}

  uielem(const uielem &) = delete;
  auto operator=(const uielem &) -> uielem & = delete;
  uielem(uielem &&) = delete;
  auto operator=(uielem &&) -> uielem & = delete;

  inline virtual ~uielem() = default;

  [[nodiscard]] inline auto parent() const -> uielem * { return parent_; }

  [[nodiscard]] inline auto name() const -> const std::string & {
    return name_;
  }

  [[nodiscard]] inline auto value() const -> const std::string & {
    return value_;
  }

  inline void set_value(const std::string &value) { value_ = value; }

  [[nodiscard]] inline auto id() const -> std::string {
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
      id.append("-").append((*it)->name());
    }
    return id;
  }

  // virtuals

  virtual void render(uiprinter &x) { x.p(value_); }

  virtual auto get_child([[maybe_unused]] const std::string &name) -> uielem * {
    throw client_exception("elem:get_child");
  }

  virtual void on_callback([[maybe_unused]] uiprinter &x,
                           [[maybe_unused]] const std::string &func,
                           [[maybe_unused]] const std::string &arg) {
    throw client_exception("elem:on_callback: no implementation");
  }

  virtual void on_event(uiprinter &x, uielem &from, const std::string &msg,
                        const int num, void *data) {
    if (parent_) {
      parent_->on_event(x, from, msg, num, data);
      return;
    }
    printf("!!! unhandled event: from=[%s] msg=[%s] num=[%d]\n",
           from.id().c_str(), msg.c_str(), num);
  }
};
} // namespace xiinux
