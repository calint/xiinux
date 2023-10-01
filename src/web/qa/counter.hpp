// reviewed: 2023-09-27
#pragma once
#include "../../session.hpp"
#include <atomic>

// used in qa/test-coverage.sh
namespace xiinux::web::qa {
class counter final : public widget {
  static inline std::atomic_int atomic_counter{0};
  int counter_ = 0;

  // temporary hack
  inline static const char *dup(const char *str) {
    const size_t ln = strlen(str);
    char *ret = new char[ln + 1];
    memcpy(ret, str, ln + 1);
    return ret;
  }

public:
  void to(reply &r) override {
    counter_++;
    counter::atomic_counter++;
    strb<256> sb;
    sb.p("path: ").p(r.get_path()).nl();
    sb.p("query: ").p(r.get_query()).nl();
    const map_headers &hdrs{r.get_req_headers()};
    sb.p("cookie: ").p(hdrs.at("cookie")).nl();

    map_session *ses = r.get_session();
    auto it{ses->find("x")};
    sb.p("session value: ").p(it != ses->end() ? it->second : "").nl();
    ses->insert({"x", "abc"});

    sb.p("counter in this instance: ").p(counter_).nl();
    sb.p("counter in this class: ").p(counter::atomic_counter).nl();
    r.http(200, sb.buf(), sb.len(), "text/plain");
  }
};
} // namespace xiinux::web::qa
