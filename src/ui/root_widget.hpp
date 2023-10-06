#include "../widget.hpp"
#include "elem.hpp"

namespace xiinux::ui {
class root_widget final : public widget {
  std::unique_ptr<elem> elem_;

public:
  inline explicit root_widget(std::unique_ptr<elem> el)
      : elem_{std::move(el)} {}
  root_widget(const root_widget &) = delete;
  auto operator=(const root_widget &) -> root_widget & = delete;
  root_widget(root_widget &&) = default;
  auto operator=(root_widget &&) -> root_widget & = default;
  ~root_widget() override = default;

  inline void to(reply &r) override { elem_->to(*r.reply_chunky()); }

  // called when client is sending content
  // first call is with buf=nullptr, buf_len=0, received_len=0 and content_len
  // subsequent calls are the portions of the content as read from the client
  // received_len keeps track of received content this far
  inline void on_content([[maybe_unused]] reply &x,
                         /*scan*/ [[maybe_unused]] const char *buf,
                         [[maybe_unused]] const size_t buf_len,
                         [[maybe_unused]] const size_t received_len,
                         [[maybe_unused]] const size_t content_len) override {}
};
} // namespace xiinux::ui
