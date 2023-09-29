#pragma once
#include "../../widget.hpp"

namespace xiinux::web::qa {
class page final : public widget {
  strb<8 * K> txt{};

public:
  void to(reply &x) override {
    strb<32 * K> s;
    constexpr char html5_preamble[] =
        "<!doctype html><script src=/x.js></script><link "
        "rel=stylesheet href=/x.css>";
    // -1 don't copy the terminating '\0'
    // 7, 4 and 8 are string lengths
    s.p(html5_preamble, sizeof(html5_preamble) - 1);
    s.p("<title>", 7).p("page", 4).p("</title>", 8);

    s.p("Type JavaScript to execute on client <input id=_btn type=button "
        "value=run "
        "onclick=\"this.disabled=true;ajax_post('/qa/"
        "page',$('_txt').value,function(r){console.log(r.responseText);$('_btn'"
        ").disabled=false;eval(r.responseText);})\">")
        .p("\n")
        .p("<textarea id=_txt class=big>")
        .p(txt)
        .p("</textarea>")
        .p("<script>$('_txt').focus()</script>")
        .nl();
    x.http(200, s.buf(), s.len());
  }

  void on_content(reply &x, /*scan*/ const char *buf, const size_t buf_len,
                  const size_t received_len,
                  const size_t content_len) override {

    if (buf == nullptr) { // begin content receive
      // non-empty content is being sent
      txt.rst();
      return;
    }

    // add content to 'txt'
    txt.p(buf, buf_len);

    if (received_len == content_len) { // last call?
      x.http(200, txt.buf(), txt.len(), "text/plain;charset=utf-8");
    }
  }
};
} // namespace xiinux::web::qa
