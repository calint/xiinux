// reviewed: 2023-09-27
#pragma once
#include "../../session.hpp"
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
    sb.p("path: ").p(r.get_path()).nl();
    sb.p("query: ").p(r.get_query()).nl();
    sb.p("cookie: ").p(r.get_req_headers()["cookie"]).nl();
    sb.p("session value: ").p(r.get_session()->get("x")).nl();
    r.get_session()->put("x", "abc");
    sb.p("counter in this instance: ").p(counter_).nl();
    sb.p("counter in this class: ").p(counter::atomic_counter).nl();
    r.http(200, sb.buf(), sb.len(), "text/plain");
  }
};
} // namespace xiinux::web::qa
