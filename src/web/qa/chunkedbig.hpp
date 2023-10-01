// reviewed: 2023-09-27
#pragma once
#include <memory>

// used in qa/test-coverage.sh
namespace xiinux::web::qa {
class chunkedbig final : public widget {
public:
  void to(reply &r) override {
    auto x = r.reply_chunky("text/plain;charset=utf-8"sv);
    for (unsigned i = 0; i < 4 * 1024; i++) {
      x->p(" chunked response "sv).p(int(i));
    }
  }
};
} // namespace xiinux::web::qa
