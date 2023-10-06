#include "../widget.hpp"
#include "elem.hpp"
#include <iostream>
#include <vector>

namespace xiinux::ui {
class root_widget final : public widget {
  std::unique_ptr<elem> elem_;
  std::string content_{};

public:
  inline explicit root_widget(std::unique_ptr<elem> el)
      : elem_{std::move(el)} {}
  root_widget(const root_widget &) = delete;
  auto operator=(const root_widget &) -> root_widget & = delete;
  root_widget(root_widget &&) = default;
  auto operator=(root_widget &&) -> root_widget & = default;
  ~root_widget() override = default;

  inline void to(reply &r) override { elem_->render(*r.reply_chunky()); }

  // called when client is sending content
  // first call is with buf=nullptr, buf_len=0, received_len=0 and content_len
  // subsequent calls are the portions of the content as read from the client
  // received_len keeps track of received content this far
  inline void on_content([[maybe_unused]] reply &x,
                         /*scan*/ [[maybe_unused]] const char *buf,
                         [[maybe_unused]] const size_t buf_len,
                         [[maybe_unused]] const size_t received_len,
                         [[maybe_unused]] const size_t content_len) override {
    if (buf == nullptr) {
      content_.clear();
      return;
    }
    content_.append(buf, buf_len);
    if (received_len != content_len) {
      return;
    }

    printf("\n\ncontent received, process:\n");
    std::string callback_id{};
    std::string callback_func{};
    std::string callback_arg{};
    // first line is callback info
    std::string line;
    std::istringstream iss_content{content_};
    std::getline(iss_content, line, '\r');
    const std::size_t ix_first_space = line.find(' ');
    if (ix_first_space == std::string::npos) {
      callback_id = line;
      callback_func = "_";
    } else {
      callback_id = line.substr(0, ix_first_space);
      line = line.substr(ix_first_space + 1);
      const std::size_t ix_second_space = line.find(' ');
      if (ix_second_space == std::string::npos) {
        callback_func = line;
      } else {
        callback_id = line.substr(0, ix_second_space);
        callback_arg = line.substr(ix_second_space + 1);
      }
    }
    // extract the elem id of callback target and callback parameter
    std::cout << "callback id=[" << callback_id << "] arg=[" << callback_arg
              << "]\n";

    while (std::getline(iss_content, line, '\r')) {
      // std::cout << "line=[" << line << "]" << std::endl;
      const std::size_t ix = line.find('=');
      if (ix == std::string::npos) {
        throw client_exception("root_widget:on_content: postback malformed 2");
      }
      const std::string id = line.substr(0, ix);
      const std::string value = line.substr(ix + 1);
      std::cout << "set elem value id=[" << id << "] value=[" << value << "]\n";

      const std::vector<std::string> id_split_vec = split_string(id, '-');
      elem *el = elem_.get();
      for (auto it = id_split_vec.begin() + 1; it != id_split_vec.end(); ++it) {
        el = el->get_child(*it);
        if (!el) {
          throw client_exception("root_widget:on_content");
        }
      }
      el->set_value(value);
    }

    const std::vector<std::string> id_split_vec = split_string(callback_id, '-');
    elem *el = elem_.get();
    for (auto it = id_split_vec.begin() + 1; it != id_split_vec.end(); ++it) {
      el = el->get_child(*it);
      if (!el) {
        throw client_exception("root_widget:on_content");
      }
    }
    auto out = x.reply_chunky();
    el->on_callback(*out, callback_func, callback_arg);
  }

private:
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
} // namespace xiinux::ui
