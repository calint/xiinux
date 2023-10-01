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
    sb.p("path: "sv).p(r.get_path()).nl();
    sb.p("query: "sv).p(r.get_query()).nl();

    const map_headers &hdrs{r.get_req_headers()};
    sb.p("cookie: "sv).p(hdrs.at("cookie"sv )).nl();

    map_session *ses = r.get_session();
    auto it{ses->find("x"s)};
    sb.p("session value: "sv).p(it != ses->end() ? it->second : ""sv).nl();
    ses->insert({"x"s, "abc"s});

    sb.p("counter in this instance: "sv).p(counter_).nl();
    sb.p("counter in this class: "sv).p(counter::atomic_counter).nl();
    
    r.http(200, sb.string_view(), "text/plain"sv);
  }
};
} // namespace xiinux::web::qa
