// reviewed: 2023-09-27
#pragma once
#include "../../widget.hpp"
#include <atomic>

// used in qa/test-coverage.sh
namespace xiinux::web::qa {
class counter final : public widget {
    static inline std::atomic_int atomic_counter{0};
    int counter_ = 0;

  public:
    auto to(reply& r) -> void override {
        counter_++;
        counter::atomic_counter++;

        strb<256> sb{};
        sb.p("path: ").p(r.get_path()).nl();
        sb.p("query: ").p(r.get_query()).nl();

        const map_headers& hdrs{r.get_req_headers()};
        sb.p("cookie: ").p(hdrs.at("cookie")).nl();

        map_session* ses = r.get_session();

        // auto it = ses->find("x"s);
        // const bool found = it != ses->end();
        // sb.p("session value: ").p(found ? it->second : "").nl();
        // ses->insert({"x"s, "abc"s});

        // throws exception if not found
        // const std::string &ses_val = ses->at("x");

        // inserts empty string if not found
        const std::string& ses_val = (*ses)["x"];
        sb.p("session value: ").p(ses_val).nl();
        (*ses)["x"] = "abc";

        // ses->insert_or_assign("x"s, "abc"s);

        sb.p("counter in this instance: ").p(counter_).nl();
        sb.p("counter in this class: ").p(counter::atomic_counter).nl();

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) coverage
        // test
        sb.p("pointer: ").p_ptr(reinterpret_cast<void*>(0x1234abcd)).nl();

        r.http(200, sb.string_view(), "text/plain");
    }
};
} // namespace xiinux::web::qa
