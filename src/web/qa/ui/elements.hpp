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
    const std::string &eid = id();
    x.input_text(a.id(), a.value(), "S", eid, "").nl();
    x.textarea(b.id(), b.value(), "L").nl();
    x.input_text(c.id(), c.value(), "S", eid, "foo x y").nl();
    x.output(d.id(), "").nl();
    x.button(eid, "", "arg1 arg2", "attention", "submit");
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
    x.xalert(arg);
    x.xalert("["s + a.value() + "] ["s + b.value() + "]"s);
  }

  void x_foo(uiprinter &x, [[maybe_unused]] const std::string &arg) {
    x.xset(d.id(), a.value() + " " + b.value());
    x.xalert("foo "s + arg);
  }
};
} // namespace xiinux::web::qa::ui
