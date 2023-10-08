#pragma once

namespace xiinux::web::qa::ui {
class menu3 : public uielem {
public:
  int selected_ix{};
  std::vector<std::string> items{};

  inline menu3(uielem *parent, std::string name)
      : uielem{parent, std::move(name)} {}

  inline void render(uiprinter &x) override {
    int ix = 0;
    const std::string &eid = id();
    for (const auto &itm : items) {
      std::string cls{};
      if (ix == selected_ix) {
        cls = "attention";
      }
      x.button(eid, "sel", std::to_string(ix), cls, itm).p(' ');
      ix++;
    }
  }

  inline void on_callback(uiprinter &x, const std::string &func,
                          const std::string &arg) override {
    if (func == "sel") {
      selected_ix = std::atoi(arg.c_str());
      uielem::on_event(x, *this, selected_ix, "sel", nullptr);
      return;
    }
    throw client_exception("unhandled callback");
  }
};
} // namespace xiinux::web::qa::ui
