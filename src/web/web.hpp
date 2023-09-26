//-- generated (todo)
#pragma once
#include "chunked.hpp"
#include "chunkedbig.hpp"
#include "chunkedbigger.hpp"
#include "counter.hpp"
#include "hello.hpp"
#include "notfound.hpp"
#include "page.hpp"
#include "typealine.hpp"
namespace xiinux {
static inline widget *widgetget(const char *qs) {
  //?? "/?hello"  vs "/?hello&a=1"
  if (!strcmp("hello", qs))
    return new web::hello();
  if (!strcmp("typealine", qs))
    return new web::typealine();
  if (!strcmp("counter", qs))
    return new web::counter();
  if (!strcmp("page", qs))
    return new web::page();
  if (!strcmp("chunked", qs))
    return new web::chunked();
  if (!strcmp("chunkedbig", qs))
    return new web::chunkedbig();
  if (!strcmp("chunkedbigger", qs))
    return new web::chunkedbigger();
  return new web::notfound();
}
} // namespace xiinux
