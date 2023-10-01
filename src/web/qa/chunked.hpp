// reviewed: 2023-09-27
#pragma once
#include "../../widget.hpp"
#include <memory>

// used in qa/test-coverage.sh
namespace xiinux::web::qa {
class chunked final : public widget {
public:
  void to(reply &r) override {
    auto x = r.reply_chunky("text/plain;charset=utf-8"sv);
    x->p("chunked response"sv).nl();
  }
};
} // namespace xiinux::web::qa
