#pragma once
#include "../../../ui/uiprinter.hpp"

namespace xiinux::web::qa::ui {
class elements final : public xiinux::ui::elem {
  elem a{this, "a"};
  elem b{this, "b"};

public:
  inline explicit elements(const std::string &name) : elem{nullptr, name} {}
  void render(xiinux::ui::uiprinter &x) override {
    x.p("<pre>hello world from elements\n"sv);
    const std::string &id = get_id();
    x.input_text(a.get_id(), a.get_value(), id);
    x.nl();
    x.textarea(b.get_id(), b.get_value());
    x.nl();
    x.button(id, "foo", "arg1 arg2", "submit");
  }

  auto get_child(const std::string &name) -> elem * override {
    if (name == "a") {
      return &a;
    }
    if (name == "b") {
      return &b;
    }
    return nullptr;
  }

  void on_callback(xiinux::ui::uiprinter &x, const std::string &name,
                   const std::string &param) override {
    x.p("alert('").p(name).p("(").p(param).p(")');\n");
    x.p("alert('").p(a.get_value()).p(" ").p(b.get_value()).p("');\n");
  }
};
} // namespace xiinux::web::qa::ui
