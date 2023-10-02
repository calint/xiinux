#pragma once

namespace xiinux::web::qa {
class page final : public widget {
  strb<8 * K> txt{};

public:
  void to(reply &x) override {
    strb<32 * K> s{};
    s.p("<!doctype html><script src=/x.js></script><link "
        "rel=stylesheet href=/x.css>"sv)
        .p("<title>page</title>"sv)
        .p("Type JavaScript to execute on client <input id=_btn type=button "
           "value=run onclick=\"this.disabled=true;ajax_post('/qa/"
           "page',$('_txt').value,function(r){console.log(r.responseText);$('_"
           "btn').disabled=false;eval(r.responseText);})\">\n"sv)
        .p("<textarea id=_txt class=big>"sv)
        .p(txt)
        .p("</textarea><script>$('_txt').focus()</script>"sv)
        .nl();

    x.http(200, s.string_view());
  }

  void on_content(reply &x, /*temporary*/ const char *buf, const size_t buf_len,
                  const size_t received_len,
                  const size_t content_len) override {

    if (buf == nullptr) { // begin content receive
      // non-empty content is being sent
      txt.rst();
      return;
    }

    // add content to 'txt'
    txt.p({buf, buf_len});

    if (received_len == content_len) { // last call?
      if (!txt.len()) {
        txt.p("alert('enter javascript code');"sv);
        x.http(200, txt.string_view(), "text/plain;charset=utf-8"sv);
        txt.rst();
        return;
      }
      x.http(200, txt.string_view(), "text/plain;charset=utf-8"sv);
    }
  }
};
} // namespace xiinux::web::qa
