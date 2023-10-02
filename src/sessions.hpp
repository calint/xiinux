// reviewed: 2023-09-27
#pragma once
#include "session.hpp"
#include <string_view>
#include <unordered_map>

namespace xiinux {

using map_sessions = std::unordered_map<std::string, std::unique_ptr<session>>;

class sessions final {
  map_sessions sessions_{};

public:
  inline auto get(std::string_view id) -> session * {
    return sessions_[std::string{id}].get();
  }

  inline void put(std::unique_ptr<session> ses) {
    sessions_[std::string{ses->get_id()}] = std::move(ses);
  }

} static sessions;
} // namespace xiinux
