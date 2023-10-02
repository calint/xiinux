// reviewed: 2023-09-27
#pragma once

namespace xiinux {
class xprinter {
public:
  virtual ~xprinter() = default;
  virtual auto p(std::string_view sv) -> xprinter & = 0;
  virtual auto p(char ch) -> xprinter & = 0;
  virtual auto p(int i) -> xprinter & = 0;
  virtual auto p(size_t sz) -> xprinter & = 0;
  virtual auto p_ptr(const void *ptr) -> xprinter & = 0;
  virtual auto p_hex(int i) -> xprinter & = 0;
  virtual auto nl() -> xprinter & = 0;
};
} // namespace xiinux
