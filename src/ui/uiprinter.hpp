#pragma once

namespace xiinux::ui {
class uiprinter : public xprinter {
  xprinter &out_;

public:
  inline explicit uiprinter(xprinter &out) : out_{out} {}

  inline auto input_text(const std::string &id, const std::string &value,
                         const std::string &on_ret_cb_id) -> uiprinter & {
    return p("<input type=text id=")
        .p(id)
        .p(" name=")
        .p(id)
        .p(" value=\"")
        .p(value)
        .p("\" ")
        .p("onkeypress=\"return $r(event,this,'")
        .p(on_ret_cb_id)
        .p("')\" oninput=\"$b(this)\">");
  }

  inline auto textarea(const std::string &id, const std::string &value)
      -> uiprinter & {
    return p("<textarea id=")
        .p(id)
        .p(" name=")
        .p(id)
        .p(" oninput=\"$b(this)\">")
        .p(value)
        .p("</textarea>");
  }

  inline auto button(const std::string &cb_id, const std::string &func,
                     const std::string &arg, const std::string &txt)
      -> uiprinter & {
    p("<button onclick=\"$x('").p(cb_id).p(' ').p(func);
    if (!arg.empty()) {
      p(' ').p(arg);
    }
    return p("')\">").p(txt).p("</button>");
  }

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
