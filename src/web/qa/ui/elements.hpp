namespace xiinux::web::qa::ui {
class elements final : public xiinux::ui::elem {
public:
  void to(xprinter &x) override { x.p("hello world from elements"sv); }
};
} // namespace xiinux::web::qa::ui
