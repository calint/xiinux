#pragma once

namespace xiinux::web::qa {
class page final : public widget {
  strb<32 * K> txt;

public:
  void to(reply &x) override {
    strb<32 * K> s;
    constexpr char html5_preamble[] =
        "<!doctype html><script src=/x.js></script><link "
        "rel=stylesheet href=/x.css>";
    // -1 to not copy the terminator \0
    // 7, 4 and 8 are the number of characters to copy
    s.p(html5_preamble, sizeof(html5_preamble) - 1);
    s.p("<title>", 7).p("page", 4).p("</title>", 8);
    s.p("<input id=_btn type=button value=update "
        "onclick=\"this.disabled=true;ajax_post('/"
        "?page',$('_txt').value,function(r){console.log(r);$('_btn')."
        "disabled=false;eval(r.responseText);})\">")
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
      txt.rst();
      return;
    }

    txt.p(buf, buf_len);

    if (received_len == content_len) {
      x.http(200, "location.reload();");
    }
  }
};
} // namespace xiinux::web::qa
