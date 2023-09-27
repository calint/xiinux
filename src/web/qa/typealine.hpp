// reviewed: 2023-09-27
#pragma once
// used in qa/test-coverage.sh
namespace xiinux::web::qa {
class typealine final: public widget {

  void to(reply &x) override { x.http(403, "forbidden. intended for post only"); }

  void on_content(reply &x, /*scan*/ const char *buf, const size_t buf_len,
                  const size_t received_len,
                  const size_t content_len) override {
    if (!buf) { // begin content receive
      strb<128> sb;
      sb.p("HTTP/1.1 200\r\nContent-Length: ").p(content_len).p("\r\n\r\n");
      x.send(sb.buf(), sb.len(), true, true);
      return;
    }
    x.send(buf, buf_len, true, received_len != content_len);
  }
};
} // namespace xiinux::web::qa
