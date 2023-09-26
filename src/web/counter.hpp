#pragma once
#include <atomic>
// used in qa/test-coverage.sh
namespace xiinux::web::qa {
class counter final : public widget {
  static inline std::atomic_int atomic_counter{0};
  int counter_ = 0;

public:
  void to(reply &r) override {
    counter_++;
    counter::atomic_counter++;
    strb<256> sb;
    sb.p("counter in this instance: ").p(counter_).nl();
    sb.p("counter in this class: ").p(counter::atomic_counter).nl();
    r.http(200, sb.buf(), sb.size());
  }
};
} // namespace xiinux::web::qa
