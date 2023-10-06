// reviewed: 2023-09-27
#pragma once

namespace xiinux {

// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions,hicpp-special-member-functions)
class xprinter {
public:
  virtual ~xprinter() = default;

  virtual auto p(const std::string_view &sv) -> xprinter & = 0;
  virtual auto p(char ch) -> xprinter & = 0;
  virtual auto p(int i) -> xprinter & = 0;
  virtual auto p(size_t sz) -> xprinter & = 0;
  virtual auto p_ptr(const void *ptr) -> xprinter & = 0;
  virtual auto p_hex(int i) -> xprinter & = 0;
  virtual auto nl() -> xprinter & = 0;
  virtual auto flush() -> xprinter & = 0;
};

} // namespace xiinux
