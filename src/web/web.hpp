// reviewed: 2023-09-27
// todo: generate this file from index
#pragma once
#include "../lut.hpp"
#include "error404.hpp"
#include "qa/chunked.hpp"
#include "qa/chunkedbig.hpp"
#include "qa/chunkedbigger.hpp"
#include "qa/counter.hpp"
#include "qa/hello.hpp"
#include "qa/page.hpp"
#include "qa/typealine.hpp"

namespace xiinux::web {

inline static lut<widget *(*)()> widget_path_to_factory_map{};

static inline widget *(*widget_factory_for_path(const char *path))() {
  return widget_path_to_factory_map[path];
}

static inline widget *widget_create_chunked() { return new qa::chunked(); }
static inline widget *widget_create_chunkedbig() {
  return new qa::chunkedbig();
}
static inline widget *widget_create_chunkedbigger() {
  return new qa::chunkedbigger();
}
static inline widget *widget_create_counter() { return new qa::counter(); }
static inline widget *widget_create_hello() { return new qa::hello(); }
static inline widget *widget_create_page() { return new qa::page(); }
static inline widget *widget_create_typealine() { return new qa::typealine(); }

static inline void widget_init_path_to_factory_map() {
  widget_path_to_factory_map.put("/qa/hello", widget_create_hello);
  widget_path_to_factory_map.put("/qa/typealine", widget_create_typealine);
  widget_path_to_factory_map.put("/qa/counter", widget_create_counter);
  widget_path_to_factory_map.put("/qa/chunked", widget_create_chunked);
  widget_path_to_factory_map.put("/qa/chunkedbig", widget_create_chunkedbig);
  widget_path_to_factory_map.put("/qa/chunkedbigger",
                                 widget_create_chunkedbigger);
  widget_path_to_factory_map.put("/qa/page", widget_create_page);
}

} // namespace xiinux::web
