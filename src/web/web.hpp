//-- generated (todo)
#pragma once
#include "notfound.hpp"
#include "qa/chunked.hpp"
#include "qa/chunkedbig.hpp"
#include "qa/chunkedbigger.hpp"
#include "qa/counter.hpp"
#include "qa/hello.hpp"
#include "qa/page.hpp"
#include "qa/typealine.hpp"
namespace xiinux {
static inline widget *widget_new(const char *qs) {
  //?? "/?hello"  vs "/?hello&a=1"
  if (!strcmp("hello", qs))
    return new web::qa::hello();
  if (!strcmp("typealine", qs))
    return new web::qa::typealine();
  if (!strcmp("counter", qs))
    return new web::qa::counter();
  if (!strcmp("page", qs))
    return new web::qa::page();
  if (!strcmp("chunked", qs))
    return new web::qa::chunked();
  if (!strcmp("chunkedbig", qs))
    return new web::qa::chunkedbig();
  if (!strcmp("chunkedbigger", qs))
    return new web::qa::chunkedbigger();
  return new web::notfound();
}
} // namespace xiinux
