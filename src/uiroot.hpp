#pragma once
#include "uielem.hpp"
#include "widget.hpp"
#include <iostream>
#include <vector>

namespace xiinux {
class uiroot final : public widget {
  std::unique_ptr<uielem> elem_;
  std::string content_{};

public:
  inline explicit uiroot(std::unique_ptr<uielem> el) : elem_{std::move(el)} {}

  uiroot(const uiroot &) = delete;
  auto operator=(const uiroot &) -> uiroot & = delete;
  uiroot(uiroot &&) = delete;
  auto operator=(uiroot &&) -> uiroot & = delete;

  ~uiroot() override = default;

  inline void to(reply &r) override {
    std::unique_ptr<chunky> z = r.reply_chunky();
    uiprinter out{*z};
    out.p(
        R"(<!doctype html><meta name=viewport content="width=device-width,initial-scale=1"><meta charset=utf-8><link rel=stylesheet href=/ui/x.css><script src=/ui/x.js></script>)");
    elem_->render(out);
  }

  inline void on_content(reply &x, const char *buf, const size_t buf_len,
                         const size_t received_len,
                         const size_t content_len) override {
    if (buf == nullptr) {
      content_.clear();
      return;
    }

    content_.append(buf, buf_len);
    if (received_len != content_len) {
      return;
    }

    // first line is callback info e.g. "--a func arg1 arg2\r"
    std::string callback_id{};
    std::string callback_func{};
    std::string callback_arg{};
    std::string line;
    std::istringstream iss_content{content_};
    std::getline(iss_content, line, '\r');
    const std::size_t ix_first_space = line.find(' ');
    if (ix_first_space == std::string::npos) {
      callback_id = line;
    } else {
      callback_id = line.substr(0, ix_first_space);
      line = line.substr(ix_first_space + 1);
      const std::size_t ix_second_space = line.find(' ');
      if (ix_second_space == std::string::npos) {
        callback_func = line;
      } else {
        callback_func = line.substr(0, ix_second_space);
        callback_arg = line.substr(ix_second_space + 1);
      }
    }

    // set values posted by ui. e.g "--a hello world\r"
    while (std::getline(iss_content, line, '\r')) {
      const std::size_t ix = line.find('=');
      if (ix == std::string::npos) {
        throw client_exception("uiroot:on_content: postback malformed 2");
      }
      const std::string id = line.substr(0, ix);
      const std::string value = line.substr(ix + 1);
      get_elem_by_id(id)->set_value(value);
    }

    // do callback
    std::unique_ptr<chunky> z = x.reply_chunky();
    uiprinter out{*z};
    get_elem_by_id(callback_id)->on_callback(out, callback_func, callback_arg);
  }

private:
  inline auto get_elem_by_id(const std::string &id) -> uielem * {
    const std::vector<std::string> id_split_vec = split_string(id, '-');
    uielem *el = elem_.get();

    // for (const auto &eid : id_split_vec | std::views::drop(2)) {
    int ix = 0;
    for (const auto &eid : id_split_vec) {
      if (ix++ < 2) { // skip the first 2 elements to start from root
        continue;
      }
      el = el->get_child(eid);
      if (!el) {
        throw client_exception("uiroot:on_content");
      }
    }
    return el;
  }

  // todo: move to a util
  static auto split_string(const std::string &input, char delimiter)
      -> std::vector<std::string> {
    std::vector<std::string> result;
    std::size_t startPos = 0;
    std::size_t endPos = input.find(delimiter);

    while (endPos != std::string::npos) {
      const std::string token = input.substr(startPos, endPos - startPos);
      result.push_back(token);
      startPos = endPos + 1;
      endPos = input.find(delimiter, startPos);
    }

    const std::string lastToken = input.substr(startPos);
    result.push_back(lastToken);

    return result;
  }
};
} // namespace xiinux
