namespace xiinux::ui {
class elem {
  std::string value_{};

public:
  elem() = default;
  elem(const elem &) = default;
  auto operator=(const elem &) -> elem & = default;
  elem(elem &&) = default;
  auto operator=(elem &&) -> elem & = default;
  virtual ~elem() = default;

  void set_value(const std::string &value) { value_ = value; }

  auto get_value() -> const std::string & { return value_; }

  virtual void to(xprinter &x) = 0;

  virtual void on_callback(xprinter &x, const std::string &name,
                           const std::string &param) {
    x.p("callback id=[").p(name).p("] param=[").p(param).p("]\n");
  }
};
} // namespace xiinux::ui
