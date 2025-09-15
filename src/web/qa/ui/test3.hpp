#pragma once
#include "../../../uijstr.hpp"
#include "menu3.hpp"

namespace xiinux::web::qa::ui {
class test3 final : public uielem {
    menu3 mn{this, "mn"};
    uielem txt{this, "txt"};

  public:
    inline auto get_child(const std::string& name) -> uielem& override {
        if (name == "mn") {
            return mn;
        }
        if (name == "txt") {
            return txt;
        }
        throw client_exception{"child not found"};
    }

    inline test3() : uielem{nullptr, ""} {
        mn.items.emplace_back("item 1");
        mn.items.emplace_back("item 2");
        mn.items.emplace_back("item 3");
        mn.items.emplace_back("item 4");
    }

    inline auto render(uiprinter& x) -> void override {
        x.p("<pre>").nl();
        x.elem_open("div", mn.id(), "");
        mn.render(x);
        x.elem_close("div");
        x.nl();
        x.elem_open("div", txt.id(), "");
        txt.render(x);
        x.elem_close("div");
    }

    inline auto on_event(uiprinter& x, uielem& from, const int num,
                         const std::string& msg, void* data) -> void override {
        if (&from == &mn) {
            txt.set_value(std::to_string(num));
            x.xset(txt.id(), txt.value());
            uijstr js{x, mn.id()};
            mn.render(js);
            js.close();
            return;
        }
        uielem::on_event(x, from, num, msg, data);
    }
};
} // namespace xiinux::web::qa::ui
