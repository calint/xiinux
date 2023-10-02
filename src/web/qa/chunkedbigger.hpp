// reviewed: 2023-09-27
#pragma once

// used in qa/test-coverage.sh
namespace xiinux::web::qa {
class chunkedbigger final : public widget {
public:
  void to(reply &r) override {
    auto x = r.reply_chunky("text/plain;charset=utf-8"sv);
    constexpr size_t buf_len = K * K;
    strb<buf_len> sb{};
    int i = 0;
    while (sb.len() + 128 < buf_len) {
      sb.p(i++).p(' ');
    }
    x->p(sb.string_view());
  }
};
} // namespace xiinux::web::qa
