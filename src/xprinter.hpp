// reviewed: 2023-09-27
#pragma once

namespace xiinux {
class xprinter {
public:
  virtual ~xprinter() {}
  virtual xprinter &p(const std::string_view sv) = 0;
  virtual xprinter &p(const char ch) = 0;
  virtual xprinter &p(const int i) = 0;
  virtual xprinter &p(const size_t sz) = 0;
  virtual xprinter &p_ptr(const void *ptr) = 0;
  virtual xprinter &p_hex(const int i) = 0;
  virtual xprinter &nl() = 0;
};
} // namespace xiinux
