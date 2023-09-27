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
  
  void on_content(reply &x, /*scan*/ const char *content,
                  const size_t content_len,
                  const size_t total_content_len) override {

    if (content == nullptr) {
      txt.rst();
      return;
    }

    txt.p(content, content_len);
    
    if (txt.size() == total_content_len) {
      x.http(200, "location.reload();");
    }
  }
};
} // namespace xiinux::web::qa
