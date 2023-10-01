// reviewed: 2023-09-27
// todo: generate this file from index
#pragma once
#include "../lut.hpp"
#include "error404.hpp"
#include "qa/bigresp.hpp"
#include "qa/chunked.hpp"
#include "qa/chunkedbig.hpp"
#include "qa/chunkedbigger.hpp"
#include "qa/counter.hpp"
#include "qa/hello.hpp"
#include "qa/page.hpp"
#include "qa/typealine.hpp"

namespace xiinux::web {

using map_widget_factories = std::unordered_map<std::string, widget *(*)()>;
inline static map_widget_factories path_to_widget_map{};
inline static lut<widget *(*)(), false, false> widget_path_to_factory_map{
    conf::path_to_widget_lut_size};

static inline widget *(*widget_factory_for_path(const std::string &path))() {
  auto it = path_to_widget_map.find(path);
  if (it != path_to_widget_map.end())
    return it->second;
  return nullptr;
}

static inline widget *widget_create_bigresp() { return new qa::bigresp(); }
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
  path_to_widget_map["/qa/typealine"] = widget_create_typealine;
  path_to_widget_map["/qa/bigresp"] = widget_create_bigresp;
  path_to_widget_map["/qa/hello"] = widget_create_hello;
  path_to_widget_map["/qa/counter"] = widget_create_counter;
  path_to_widget_map["/qa/chunked"] = widget_create_chunked;
  path_to_widget_map["/qa/chunkedbig"] = widget_create_chunkedbig;
  path_to_widget_map["/qa/chunkedbigger"] = widget_create_chunkedbigger;
  path_to_widget_map["/qa/page"] = widget_create_page;
}

} // namespace xiinux::web
