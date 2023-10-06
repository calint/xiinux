#pragma once

namespace xiinux {
class uiprinter : public xprinter {
  xprinter &out_;

public:
  inline explicit uiprinter(xprinter &out) : out_{out} {}

  inline auto input_text(const std::string &id, const std::string &value,
                         const std::string &cls,
                         const std::string &on_ret_cb_id,
                         const std::string &cb_func) -> uiprinter & {
    p("<input id=").p(id).p(" value=\"").p(value).p("\"");
    if (!cls.empty()) {
      p(" class=\"").p(cls).p("\"");
    }
    if (!on_ret_cb_id.empty()) {
      p(" type=text onkeypress=\"return $r(event,this,'").p(on_ret_cb_id);
      if (!cb_func.empty()) {
        p(' ').p(cb_func);
      }
      p("')\"");
    }
    p(" oninput=\"$b(this)\" ").p(" name=").p(id).p('>');
    return *this;
  }

  inline auto textarea(const std::string &id, const std::string &value,
                       const std::string &cls) -> uiprinter & {
    p("<textarea id=").p(id);
    if (!cls.empty()) {
      p(" class=\"").p(cls).p('"');
    }
    p(" oninput=\"$b(this)\" name=").p(id).p('>').p(value).p("</textarea>");
    return *this;
  }

  inline auto button(const std::string &cb_id, const std::string &func,
                     const std::string &arg, const std::string &cls,
                     const std::string &txt) -> uiprinter & {
    p("<button onclick=\"$x('").p(cb_id).p(' ').p(func);
    if (!arg.empty()) {
      p(' ').p(arg);
    }
    p("')\"");
    if (!cls.empty()) {
      p(" class=\"").p(cls).p("\"");
    }
    return p(">").p(txt).p("</button>");
  }

  inline auto output(const std::string &id, const std::string &html)
      -> uiprinter & {
    return p("<output id=")
        .p(id)
        .p(" name=")
        .p(id)
        .p(">")
        .p(html)
        .p("</output>");
  }

  inline auto xalert(const std::string &msg) -> uiprinter & {
    return p("ui.alert('").p_js_str(msg).p("');").nl();
  }

  inline auto xset(const std::string &id, const std::string &value)
      -> uiprinter & {
    return p("$sv('").p(id).p("','").p_js_str(value).p("');").nl();
  }

  inline auto p_js_str(const std::string &str) -> uiprinter & {
    return p(js_str(str));
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

private:
  // note. temporary hack
  static auto js_str(const std::string &s0) -> std::string {
    const std::string s1 = replace_char_with_string(s0, '\n', "\\n");
    const std::string s2 = replace_char_with_string(s1, '\r', "\\r");
    const std::string s3 = replace_char_with_string(s2, '\0', "\\0");
    return replace_char_with_string(s3, '\'', "\\'");
  }

  static auto replace_char_with_string(const std::string &str, char ch,
                                       const std::string &replacement)
      -> std::string {
    std::string res = str;
    size_t pos = res.find(ch);
    while (pos != std::string::npos) {
      res.replace(pos, 1, replacement);
      pos = res.find(ch, pos + replacement.length());
    }
    return res;
  }
};
} // namespace xiinux
