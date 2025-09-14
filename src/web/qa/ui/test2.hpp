#pragma once
#include "../../../uijstr.hpp"
#include "menu.hpp"

namespace xiinux::web::qa::ui {
class test2 : public uielem {
    menu mn{this, "mn"};

  public:
    inline auto get_child(const std::string& name) -> uielem* override {
        if (name == "mn") {
            return &mn;
        }
        return nullptr;
    }

    inline test2() : uielem{nullptr, ""} {}

    inline void render(uiprinter& x) override {
        x.p("<pre>").nl();
        const std::string& eid = id();
        x.button(eid, "sel", "0", "", "select 1");
        x.button(eid, "sel", "1", "", "select 2");
        x.button(eid, "sel", "2", "", "select 3");
        x.button(eid, "sel", "3", "", "select 4");
        x.nl();

        x.elem_open("div", mn.id(), "");
        mn.render(x);
        x.elem_close("div");
    }

    inline void on_callback(uiprinter& x, const std::string& func,
                            const std::string& arg) override {
        if (func == "sel") {
            x_sel(x, arg);
        } else {
            throw client_exception("elements:on_callback: func not found");
        }
    }

  private:
    inline void x_sel(uiprinter& x, const std::string& arg) {
        mn.selected_ix = unsigned(std::stoi(arg));

        uijstr z{x, mn.id()};
        mn.render(z);
        z.close();
    }
};
} // namespace xiinux::web::qa::ui
