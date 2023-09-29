// reviewed: 2023-09-27
// todo: generate this file from index
#pragma once
#include "error404.hpp"
#include "qa/chunked.hpp"
#include "qa/chunkedbig.hpp"
#include "qa/chunkedbigger.hpp"
#include "qa/counter.hpp"
#include "qa/hello.hpp"
#include "qa/page.hpp"
#include "qa/typealine.hpp"
#include <functional>

namespace xiinux::web {

inline static lut<std::function<widget *()>> path_to_widget_factory_map{};

static inline void init_path_to_widget_factory_map() {
  path_to_widget_factory_map.put(
      "/qa/hello", []() -> widget * { return new xiinux::web::qa::hello(); });
  path_to_widget_factory_map.put("/qa/typealine", []() -> widget * {
    return new xiinux::web::qa::typealine();
  });
  path_to_widget_factory_map.put("/qa/counter", []() -> widget * {
    return new xiinux::web::qa::counter();
  });
  path_to_widget_factory_map.put(
      "/qa/page", []() -> widget * { return new xiinux::web::qa::page(); });
  path_to_widget_factory_map.put("/qa/chunked", []() -> widget * {
    return new xiinux::web::qa::chunked();
  });
  path_to_widget_factory_map.put("/qa/chunkedbig", []() -> widget * {
    return new xiinux::web::qa::chunkedbig();
  });
  path_to_widget_factory_map.put("/qa/chunkedbigger", []() -> widget * {
    return new xiinux::web::qa::chunkedbigger();
  });
}

static inline bool widget_factory_is_mapped_to_path(const char *path) {
  return path_to_widget_factory_map.has(path);
}

static inline const std::function<widget *()> &
widget_factory_for_path(const char *path) {
  return path_to_widget_factory_map.get_ref_const(path);
}

} // namespace xiinux::web
