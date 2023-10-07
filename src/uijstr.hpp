#pragma once
#include "uiprinter.hpp"

namespace xiinux {
class uijstr final : public uiprinter {
  bool closed_{};

public:
  explicit uijstr(uiprinter &out, const std::string &elem_id)
      : uiprinter{out.get_xprinter()} {
    uiprinter::p("$s('");
    uiprinter::p(elem_id);
    uiprinter::p("','");
  }

//   inline ~uijstr() override {
//     if (closed_) {
//       return;
//     }
//     close();
//   }

  // xprinter implementation

  using uiprinter::p;

  inline auto p(const std::string_view &str) -> uijstr & override {
    size_t mark = 0;
    for (size_t ix = 0; ix < str.size(); ix++) {
      const char ch = str[ix];
      if (ch == '\n' || ch == '\r' || ch == '\'' || ch == '\0') {
        uiprinter::p(str.substr(mark, ix - mark));
        uiprinter::p('\\');
        switch (ch) {
        case '\n':
          uiprinter::p('n');
          break;
        case '\r':
          uiprinter::p('r');
          break;
        case '\'':
          uiprinter::p('\'');
          break;
        case '\0':
          uiprinter::p('0');
          break;
        default:
          throw client_exception("unexpected char");
        }
        mark = ix + 1;
      }
    }
    uiprinter::p(str.substr(mark, str.size() - mark));
    return *this;
  }

  inline auto p(const char ch) -> uijstr & override {
    if (ch == '\n' || ch == '\r' || ch == '\'' || ch == '\0') {
      uiprinter::p('\\');
      switch (ch) {
      case '\n':
        uiprinter::p('n');
        break;
      case '\r':
        uiprinter::p('r');
        break;
      case '\'':
        uiprinter::p('\'');
        break;
      case '\0':
        uiprinter::p('0');
        break;
      default:
        throw client_exception("unexpected char");
      }
      return *this;
    }
    uiprinter::p(ch);
    return *this;
  }

  inline void close() {
    uiprinter::p("');\n");
    closed_ = true;
  }
};
} // namespace xiinux
