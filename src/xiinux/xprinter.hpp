#pragma once
namespace xiinux {
class xprinter {
public:
  virtual ~xprinter() {}
  virtual xprinter &p(/*scan*/ const char *str) = 0;
  virtual xprinter &p(const size_t len, /*scan*/ const char *str) = 0;
  virtual xprinter &p(const int nbr) = 0;
  virtual xprinter &p(const size_t nbr) = 0;
  virtual xprinter &p_ptr(const void *ptr) = 0;
  virtual xprinter &p_hex(const unsigned long nbr) = 0;
  virtual xprinter &p(char ch) = 0;
  virtual xprinter &nl() = 0;
  virtual xprinter &html5(const char *title = "") = 0;
  virtual xprinter &flush() = 0;
};
} // namespace xiinux
