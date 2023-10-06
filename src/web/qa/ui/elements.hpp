#pragma once
#include "../../../uiprinter.hpp"

namespace xiinux::web::qa::ui {
class elements final : public uielem {
  uielem a{this, "a"};
  uielem b{this, "b"};

public:
  inline explicit elements(const std::string &name) : uielem{nullptr, name} {}

  // note. should be auto-generated
  auto get_child(const std::string &name) -> uielem * override {
    if (name == "a") {
      return &a;
    }
    if (name == "b") {
      return &b;
    }
    return nullptr;
  }

  void render(uiprinter &x) override {
    x.p("<pre>hello world from elements\n"sv);
    const std::string &id = get_id();
    x.input_text(a.get_id(), a.get_value(), id);
    x.nl();
    x.textarea(b.get_id(), b.get_value());
    x.nl();
    x.button(id, "foo", "arg1 arg2", "submit");
  }

  void on_callback(uiprinter &x, const std::string &name,
                   const std::string &param) override {
    x.p("alert('"sv).p(name).p("("sv).p(param).p(")');\n"sv);
    x.p("alert('["sv)
        .p_js_str(a.get_value())
        .p("] ["sv)
        .p_js_str(b.get_value())
        .p("]');\n"sv);
  }
};
} // namespace xiinux::web::qa::ui
