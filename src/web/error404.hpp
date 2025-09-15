// reviewed: 2023-09-27
#pragma once
#include "../widget.hpp"

namespace xiinux::web {
class error404 final : public widget {
    auto to(reply& x) -> void override {
        // 10 is string length
        x.http(404, {"not found\n", 10});
    }
};
} // namespace xiinux::web
