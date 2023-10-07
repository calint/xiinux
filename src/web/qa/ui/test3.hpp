#pragma once
#include "../../../uijstr.hpp"
#include "menu3.hpp"

namespace xiinux::web::qa::ui {
class test3 final : public uielem {
  menu3 mn{this, "mn"};
  uielem txt{this, "txt"};

public:
  // note. should be auto-generated
  inline auto get_child(const std::string &name) -> uielem * override {
    if (name == "mn") {
      return &mn;
    }
    if (name == "txt") {
      return &txt;
    }
    return nullptr;
  }

  inline test3() : uielem{nullptr, ""} {}

  inline void render(uiprinter &x) override {
    x.p("<pre>").nl();
    mn.render(x);
    x.nl();
    x.elem_open("div", txt.id(), "");
    txt.render(x);
    x.elem_close("div");
  }

  // note. should be auto-generated
  inline void on_callback(uiprinter &x, const std::string &func,
                          const std::string &arg) override {
    if (func == "sel") {
      x_sel(x, arg);
    } else {
      throw client_exception("elements:on_callback: func not found");
    }
  }

  inline void on_event(uiprinter &x, uielem &from, const std::string &msg,
                       const int num, void *data) override {
    if (&from == &mn) {
      txt.set_value(std::to_string(num));
      x.xset(txt.id(), txt.value());
      return;
    }
    uielem::on_event(x, from, msg, num, data);
  }

private:
  inline void x_sel(uiprinter &x, const std::string &arg) {
    mn.selected_ix = std::stoi(arg);

    uijstr z{x, mn.id()};
    mn.render(z);
    z.close();
  }
};
} // namespace xiinux::web::qa::ui
