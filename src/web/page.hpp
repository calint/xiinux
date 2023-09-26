#pragma once
namespace web {
using namespace xiinux;
class page final : public widget {
  strb<> txt;

public:
  void to(reply &x) override {
    strb<> s;
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
  void on_content(reply &x, /*scan*/ const char *content,
                  const size_t content_len,
                  const size_t total_content_len) override {
    txt.rst().p(content, content_len);
    x.http2(200, "location.reload();");
  }
};
} // namespace web
