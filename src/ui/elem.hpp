#include <vector>

namespace xiinux::ui {
class elem {
  elem *parent_{};
  std::string name_{};
  std::string value_{};

public:
  elem(elem *parent, std::string name)
      : parent_{parent}, name_{std::move(name)} {}
  elem(const elem &) = default;
  auto operator=(const elem &) -> elem & = default;
  elem(elem &&) = default;
  auto operator=(elem &&) -> elem & = default;
  virtual ~elem() = default;

  [[nodiscard]] inline auto get_name() const -> const std::string & {
    return name_;
  }
  
  [[nodiscard]] inline auto get_value() const -> const std::string & {
    return value_;
  }
  
  inline void set_value(const std::string &value) { value_ = value; }

  [[nodiscard]] inline auto get_id() const -> std::string {
    std::vector<const elem *> elems{};
    const elem *el = this;
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

  virtual void render(xprinter &x) { x.p(value_); }

  virtual auto get_child([[maybe_unused]] const std::string &name) -> elem * {
    throw client_exception("elem:get_child");
  }

  virtual void on_callback(xprinter &x, const std::string &name,
                           const std::string &param) {
    x.p("callback id=[").p(name).p("] param=[").p(param).p("]\n");
  }
};
} // namespace xiinux::ui
