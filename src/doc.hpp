// reviewed: 2023-09-28
#pragma once
#include "reply.hpp"

namespace xiinux {

class doc final {
    std::string str_{};

  public:
    inline explicit doc(std::string str) : str_{std::move(str)} {}
    inline auto to(const reply& x) const -> void { x.send(str_); }
};

static std::unique_ptr<doc> homepage{};

} // namespace xiinux
