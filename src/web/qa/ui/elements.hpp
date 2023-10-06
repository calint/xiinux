#pragma once
#include "../../../uiprinter.hpp"

namespace xiinux::web::qa::ui {
class elements final : public uielem {
  uielem a{this, "a"};
  uielem b{this, "b"};
  uielem c{this, "c"};
  uielem d{this, "d"};

public:
  inline explicit elements(const std::string &name) : uielem{nullptr, name} {}

  void render(uiprinter &x) override {
    x.p("<pre>hello world from elements\n"sv);
    const std::string &id = get_id();
    x.input_text(a.get_id(), a.get_value(), "S", id, "");
    x.nl();
    x.textarea(b.get_id(), b.get_value(), "L");
    x.nl();
    x.input_text(c.get_id(), c.get_value(), "S", id, "foo x y");
    x.nl();
    x.button(id, "", "arg1 arg2", "attention", "submit");
  }

  // note. should be auto-generated
  auto get_child(const std::string &name) -> uielem * override {
    if (name == "a") {
      return &a;
    }
    if (name == "b") {
      return &b;
    }
    if (name == "c") {
      return &c;
    }
    if (name == "d") {
      return &d;
    }
    return nullptr;
  }

  // note. should be auto-generated
  void on_callback(uiprinter &x, const std::string &func,
                   const std::string &arg) override {
    if (func.empty()) {
      x_default(x, arg);
    } else if (func == "foo") {
      x_foo(x, arg);
    } else {
      throw client_exception("elements:on_callback: func not found");
    }
  }

  void x_default(uiprinter &x, const std::string &arg) {
    x.p("alert('"sv).p_js_str(arg).p("');\n"sv);
    x.p("alert('["sv)
        .p_js_str(a.get_value())
        .p("] ["sv)
        .p_js_str(b.get_value())
        .p("]');\n"sv);
  }

  void x_foo(uiprinter &x, [[maybe_unused]] const std::string &arg) {
    d.set_value(a.get_value());
    x.p("alert('foo ").p_js_str(arg).p("')"sv);
  }
};
} // namespace xiinux::web::qa::ui
