// reviewed: 2023-09-27
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

    strb<256> sb{};
    sb.p("path: "sv).p(r.get_path()).nl();
    sb.p("query: "sv).p(r.get_query()).nl();

    const map_headers &hdrs{r.get_req_headers()};
    sb.p("cookie: "sv).p(hdrs.at("cookie"sv)).nl();

    map_session *ses = r.get_session();

    // auto it = ses->find("x"s);
    // const bool found = it != ses->end();
    // sb.p("session value: "sv).p(found ? it->second : ""sv).nl();
    // ses->insert({"x"s, "abc"s});

    // note. inserts empty string if not found
    const std::string &ses_val = (*ses)["x"];
    // note. throws exception if not found
    // const std::string &ses_val = ses->at("x");
    sb.p("session value: "sv).p(ses_val).nl();
    // (*ses)["x"] = "abc";
    ses->insert_or_assign("x"s, "abc"s);

    sb.p("counter in this instance: "sv).p(counter_).nl();
    sb.p("counter in this class: "sv).p(counter::atomic_counter).nl();

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) coverage test
    sb.p("pointer: "sv).p_ptr(reinterpret_cast<void *>(0x1234abcd)).nl();

    r.http(200, sb.string_view(), "text/plain"sv);
  }
};
} // namespace xiinux::web::qa
