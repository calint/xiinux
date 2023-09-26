#pragma once
#include "../xiinux/widget.hpp"
#include <memory>
// used in qa/test-coverage.sh
namespace xiinux::web {
class chunked final : public widget {
public:
  void to(reply &r) override {
    std::unique_ptr<chunky> y(/*take*/ r.reply_chunky());
    y->p("HTTP/1.1 "
         "200\r\nTransfer-Encoding:chunked\r\nContent-Type:text/"
         "plain;charset=utf-8\r\n\r\n");
    y->send_response_header();

    xprinter &x = *y;
    x.p("chunked response").nl();

    y->finish();
  }
};
} // namespace xiinux::web
