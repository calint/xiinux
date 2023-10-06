namespace xiinux::web::qa::ui {
class elements final : public xiinux::ui::elem {
  elem a{this, "a"};
  elem b{this, "b"};

public:
  inline explicit elements(const std::string &name) : elem{nullptr, name} {}
  void render(xprinter &x) override {
    x.p(R"(<!doctype html><meta name=viewport content="width=device-width,initial-scale=1"><meta charset=utf-8><link rel=stylesheet href=/ui/x.css><script src=/ui/x.js></script>)");
    x.p("<pre>hello world from elements\n"sv);
    const std::string id = get_id();
    render_input_text(x, a, id);
    x.nl();
    render_textarea(x, b);
    x.nl();
    render_button(x, id, "foo"s, "fow"s, "submit");
  }

  auto get_child(const std::string &name) -> elem * override {
    if (name == "a") {
      return &a;
    }
    if (name == "b") {
      return &b;
    }
    return nullptr;
  }

  void on_callback(xprinter &x, const std::string &name,
                   const std::string &param) override {
    x.p("alert('").p(name).p(" ").p(param).p("');\n");
    x.p("alert('").p(a.get_value()).p(" ").p(b.get_value()).p("');\n");
  }

private:
  static void render_input_text(xprinter &x, const elem &el,
                                const std::string &tgt_id) {
    const std::string &eid = el.get_id();
    x.p("<input type=text id=")
        .p(eid)
        .p(" name=")
        .p(eid)
        .p(" value=\"")
        .p(el.get_value())
        .p("\" ")
        .p("onkeypress=\"return $r(event,this,'")
        .p(tgt_id)
        .p("')\" oninput=\"$b(this)\">");
  }

  static void render_textarea(xprinter &x, const elem &el) {
    const std::string &eid = el.get_id();
    x.p("<textarea id=")
        .p(eid)
        .p(" name=")
        .p(eid)
        .p(" value=\"")
        .p(el.get_value())
        .p("\" oninput=\"$b(this)\">")
        .p(el.get_value())
        .p("</textarea>");
  }

  static void render_button(xprinter &x, const std::string &tgt_id,
                            const std::string &func, const std::string &arg,
                            const std::string &txt) {
    x.p("<button onclick=\"$x('").p(tgt_id).p(' ').p(func);
    if (!arg.empty()) {
      x.p(' ').p(arg);
    }
    x.p("')\">").p(txt).p("</button>");
  }
};
} // namespace xiinux::web::qa::ui
