// reviewed: 2023-09-27
#pragma once

// used in qa/test-coverage.sh
namespace xiinux::web::qa {
class chunked final : public widget {
public:
  void to(reply &r) override {
    auto x = r.reply_chunky("text/plain;charset=utf-8"sv);
    x->p("chunked response"sv).nl();
    x->p(1)
        .p(' ')
        .p(size_t(12345678))
        .p(' ')
        .p_hex(987654)
        .p(' ')
        //NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) coverage test
        .p_ptr(reinterpret_cast<void *>(0xfedc9876))
        .nl();
  }
};
} // namespace xiinux::web::qa
