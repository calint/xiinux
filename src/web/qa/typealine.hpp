#pragma once
// used in qa/test-coverage.sh
namespace xiinux::web::qa {
class typealine : public widget {

  void to(reply &x) override { x.http(200, "typealine"); }

  void on_content(reply &x, /*scan*/ const char *buf, const size_t buflen,
                  const size_t total_len) override {
    content_byte_count_ += buflen;
    if (!buf) { // first call
      content_byte_count_ = 0;
      strb<128> sb;
      sb.p("HTTP/1.1 200\r\nContent-Length: ").p(total_len).p("\r\n\r\n");
      x.send(sb.buf(), sb.size(), true, true);
      return;
    }
    x.send(buf, buflen, true, total_len == content_byte_count_);
  }

private:
  size_t content_byte_count_ = 0;
};
} // namespace xiinux::web::qa
