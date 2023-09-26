#pragma once
#include <atomic>
// used in qa/test-coverage.sh
namespace web {
using namespace xiinux;
class counter final : public widget {
public:
  static inline std::atomic_int page_counter{0};
  int my_counter = 0;
  void to(reply &r) override {
    my_counter++;
    counter::page_counter++;
    strb<> sb;
    sb.p("counter in this instance: ").p(my_counter).nl();
    sb.p("counter in this class: ").p(counter::page_counter).nl();
    r.http(200, sb.buf(), sb.size());
  }
};
} // namespace web
