// reviewed: 2023-09-27
#pragma once

namespace xiinux {
class xprinter {
public:
  virtual ~xprinter() {}
  virtual xprinter &p(/*scan*/ const char *str) = 0;
  virtual xprinter &p(/*scan*/ const char *str, const size_t str_len) = 0;
  virtual xprinter &p(const int i) = 0;
  virtual xprinter &p(const size_t sz) = 0;
  virtual xprinter &p_ptr(const void *ptr) = 0;
  virtual xprinter &p_hex(const unsigned hex) = 0;
  virtual xprinter &p(const char ch) = 0;
  virtual xprinter &nl() = 0;
  virtual xprinter &html5(const char *title = "") = 0;
};
} // namespace xiinux
