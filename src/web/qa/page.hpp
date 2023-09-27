#pragma once

namespace xiinux::web::qa {
class page final : public widget {
  strb<32 * K> txt;

public:
  void to(reply &x) override {
    strb<32 * K> s;
    s.html5("page")
        .p("<input id=_btn type=button value=update "
           "onclick=\"this.disabled=true;ajax_post('/"
           "?page',$('_txt').value,function(r){console.log(r);$('_btn')."
           "disabled=false;eval(r.responseText);})\">")
        .p("\n")
        .p("<textarea id=_txt class=big>")
        .p(txt)
        .p("</textarea>")
        .p("<script>$('_txt').focus()</script>")
        .nl();
    x.http(200, s.buf(), s.size());
  }

  void on_content(reply &x, /*scan*/ const char *buf, const size_t buf_len,
                  const size_t received_len, const size_t content_len) override {

    if (buf == nullptr) {
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
