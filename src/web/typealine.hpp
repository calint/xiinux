#pragma once
// used in qa/test-coverage.sh
namespace xiinux::web::qa {
class typealine : public widget {
  void to(reply &x) override { x.http(200, "typealine"); }
  void on_content(reply &x, /*scan*/ const char *buf, const size_t len,
                  const size_t total_len) override {
    if (!buf) { // init content scan
      char s[K];
      const int n =
          snprintf(s, sizeof(s), "HTTP/1.1 200\r\nContent-Length: %zu\r\n\r\n",
                   total_len);
      x.send(s, size_t(n));
      return;
    }
    x.send(buf, len);
  }
};
} // namespace xiinux::web
