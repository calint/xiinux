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

namespace xiinux::web {
static inline widget *widget_new(const char *qs) {
  if (!strcmp("hello", qs))
    return new qa::hello();
  if (!strcmp("typealine", qs))
    return new qa::typealine();
  if (!strcmp("counter", qs))
    return new qa::counter();
  if (!strcmp("page", qs))
    return new qa::page();
  if (!strcmp("chunked", qs))
    return new qa::chunked();
  if (!strcmp("chunkedbig", qs))
    return new qa::chunkedbig();
  if (!strcmp("chunkedbigger", qs))
    return new qa::chunkedbigger();
  return new error404();
}
} // namespace xiinux
