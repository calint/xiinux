#pragma once
#include "../../../uiprinter.hpp"
#include <chrono>
#include <thread>

namespace xiinux::web::qa::ui {
class test1 final : public uielem {
  uielem a{this, "a"};
  uielem b{this, "b"};
  uielem c{this, "c"};
  uielem d{this, "d"};

public:
  inline explicit test1(const std::string &name) : uielem{nullptr, name} {}

  void render(uiprinter &x) override {
    x.p("<pre>hello world from elements\n"sv);
    const std::string &eid = id();
    x.input_text(a.id(), a.value(), "S", eid, "").nl();
    x.input_text(b.id(), c.value(), "S", eid, "foo x y").nl();
    x.textarea(c.id(), b.value(), "L").nl();
    x.output(d.id(), "").nl();
    x.button(eid, "", "arg1 arg2", "attention", "submit");
    x.script_open().xfocus(a.id()).script_close();
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

  // callbacks

  void x_default(uiprinter &x, const std::string &arg) {
    x.xalert(arg);
    x.xalert("["s + a.value() + "] ["s + b.value() + "] ["s + c.value() + "]"s);
  }

  void x_foo(uiprinter &x, [[maybe_unused]] const std::string &arg) {
    x.xset(d.id(), std::to_string(std::stoi(a.value()) + std::stoi(b.value())));
    x.xp(d.id(), std::string{"\n"} + arg);
    x.xp(d.id(), "\nhello world ");
    for (unsigned i = 0; i < 3; i++) {
      const std::string i_str = std::to_string(i);
      x.xp(d.id(), i_str + " ");
      x.xtitle(i_str);
      x.flush();
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
  }
};
} // namespace xiinux::web::qa::ui
