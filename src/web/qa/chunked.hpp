#pragma once
#include "../../widget.hpp"
#include <memory>

// used in qa/test-coverage.sh
namespace xiinux::web::qa {
class chunked final : public widget {
public:
  void to(reply &r) override {
    std::unique_ptr<chunky> y(
        /*take*/ r.reply_chunky("text/plain;charset=utf-8"));
    y->send_response_header();

    xprinter &x = *y;
    x.p("chunked response").nl();

    y->finish();
  }
};
} // namespace xiinux::web::qa
