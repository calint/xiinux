// reviewed: 2023-09-27
#pragma once

namespace xiinux {
class xprinter {
public:
  virtual ~xprinter() = default;
  virtual xprinter &p(std::string_view sv) = 0;
  virtual xprinter &p(char ch) = 0;
  virtual xprinter &p(int i) = 0;
  virtual xprinter &p(size_t sz) = 0;
  virtual xprinter &p_ptr(const void *ptr) = 0;
  virtual xprinter &p_hex(int i) = 0;
  virtual xprinter &nl() = 0;
};
} // namespace xiinux
