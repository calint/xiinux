// reviewed: 2023-09-27
// todo: generate this file from index
#pragma once
#include "../uiroot.hpp"
#include "../widget.hpp"
#include "qa/bigresp.hpp"
#include "qa/chunked.hpp"
#include "qa/chunkedbig.hpp"
#include "qa/chunkedbigger.hpp"
#include "qa/counter.hpp"
#include "qa/hello.hpp"
#include "qa/page.hpp"
#include "qa/typealine.hpp"
#include "qa/ui/test1.hpp"
#include "qa/ui/test2.hpp"
#include "qa/ui/test3.hpp"

namespace xiinux::web {

using map_widget_factories =
    std::unordered_map<std::string, widget_factory_func_ptr>;

inline static map_widget_factories path_to_widget_map{};

static inline auto widget_factory_for_path(const std::string_view& path)
    -> widget_factory_func_ptr {
    auto it = path_to_widget_map.find(std::string{path}); //? creates a string
    if (it != path_to_widget_map.end()) {
        return it->second;
    }
    return nullptr;
}

static inline auto widget_create_bigresp() -> widget* {
    return new qa::bigresp();
}
static inline auto widget_create_chunked() -> widget* {
    return new qa::chunked();
}
static inline auto widget_create_chunkedbig() -> widget* {
    return new qa::chunkedbig();
}
static inline auto widget_create_chunkedbigger() -> widget* {
    return new qa::chunkedbigger();
}
static inline auto widget_create_counter() -> widget* {
    return new qa::counter();
}
static inline auto widget_create_hello() -> widget* { return new qa::hello(); }
static inline auto widget_create_page() -> widget* { return new qa::page(); }
static inline auto widget_create_typealine() -> widget* {
    return new qa::typealine();
}
static inline auto widget_create_ui_test1() -> widget* {
    return new xiinux::uiroot(std::make_unique<qa::ui::test1>());
}
static inline auto widget_create_ui_test2() -> widget* {
    return new xiinux::uiroot(std::make_unique<qa::ui::test2>());
}
static inline auto widget_create_ui_test3() -> widget* {
    return new xiinux::uiroot(std::make_unique<qa::ui::test3>());
}
static inline auto widget_init_path_to_factory_map() -> void {
    path_to_widget_map["/qa/typealine"] = widget_create_typealine;
    path_to_widget_map["/qa/bigresp"] = widget_create_bigresp;
    path_to_widget_map["/qa/hello"] = widget_create_hello;
    path_to_widget_map["/qa/counter"] = widget_create_counter;
    path_to_widget_map["/qa/chunked"] = widget_create_chunked;
    path_to_widget_map["/qa/chunkedbig"] = widget_create_chunkedbig;
    path_to_widget_map["/qa/chunkedbigger"] = widget_create_chunkedbigger;
    path_to_widget_map["/qa/page"] = widget_create_page;
    path_to_widget_map["/qa/ui/test1"] = widget_create_ui_test1;
    path_to_widget_map["/qa/ui/test2"] = widget_create_ui_test2;
    path_to_widget_map["/qa/ui/test3"] = widget_create_ui_test3;
}

} // namespace xiinux::web
