#pragma once
#include "../../../ui/uiprinter.hpp"
#include <regex>

namespace xiinux::web::qa::ui {
class elements final : public xiinux::ui::elem {
  elem a{this, "a"};
  elem b{this, "b"};

public:
  inline explicit elements(const std::string &name) : elem{nullptr, name} {}

  // note. should be auto-generated
  auto get_child(const std::string &name) -> elem * override {
    if (name == "a") {
      return &a;
    }
    if (name == "b") {
      return &b;
    }
    return nullptr;
  }

  void render(xiinux::ui::uiprinter &x) override {
    x.p("<pre>hello world from elements\n"sv);
    const std::string &id = get_id();
    x.input_text(a.get_id(), a.get_value(), id);
    x.nl();
    x.textarea(b.get_id(), b.get_value());
    x.nl();
    x.button(id, "foo", "arg1 arg2", "submit");
  }

  void on_callback(xiinux::ui::uiprinter &x, const std::string &name,
                   const std::string &param) override {
    x.p("alert('").p(name).p("(").p(param).p(")');\n");
    x.p("alert('[")
        .p(a.get_value())
        .p("] [")
        .p(js_str(b.get_value()))
        .p("]');\n");
  }

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
} // namespace xiinux::web::qa::ui
