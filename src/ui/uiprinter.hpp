#pragma once

namespace xiinux::ui {
class uiprinter : public xprinter {
  xprinter &out_;

public:
  inline explicit uiprinter(xprinter &out) : out_{out} {}

  // xprinter implementation
  inline auto p(const std::string_view &sv) -> uiprinter & override {
    out_.p(sv);
    return *this;
  }
  inline auto p(char ch) -> uiprinter & override {
    out_.p(ch);
    return *this;
  }
  inline auto p(int i) -> uiprinter & override {
    out_.p(i);
    return *this;
  }
  inline auto p(size_t sz) -> uiprinter & override {
    out_.p(sz);
    return *this;
  }
  inline auto p_ptr(const void *ptr) -> uiprinter & override {
    out_.p_ptr(ptr);
    return *this;
  }
  inline auto p_hex(int i) -> uiprinter & override {
    out_.p_hex(i);
    return *this;
  }
  inline auto nl() -> uiprinter & override {
    out_.nl();
    return *this;
  }
  inline auto flush() -> uiprinter & override {
    out_.flush();
    return *this;
  }
  // end of xprinter implementation
};
} // namespace xiinux::ui
