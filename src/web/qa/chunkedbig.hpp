// reviewed: 2023-09-27
#pragma once

// used in qa/test-coverage.sh
namespace xiinux::web::qa {
class chunkedbig final : public widget {
public:
  void to(reply &r) override {
    auto x = r.reply_chunky("text/plain;charset=utf-8");
    for (unsigned i = 0; i < 4 * K; i++) {
      x->p(" chunked response ").p(int(i));
    }
  }
};
} // namespace xiinux::web::qa
