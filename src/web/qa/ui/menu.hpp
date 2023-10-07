#pragma once

namespace xiinux::web::qa::ui {
class menu : public uielem {
public:
  unsigned selected_ix{};
  std::vector<std::string> items{};

  inline menu(uielem *parent, std::string name)
      : uielem{parent, std::move(name)} {
    items.emplace_back("item 1");
    items.emplace_back("item 2");
    items.emplace_back("item 3");
    items.emplace_back("item 4");
  }

  inline void render(uiprinter &x) override {
    unsigned ix = 0;
    for (const auto &itm : items) {
      if (ix++ == selected_ix) {
        x.p("&raquo; ");
      }
      x.p(itm).nl();
    }
  }
};
} // namespace xiinux::web::qa::ui
