// reviewed: 2023-09-27
#pragma once
#include "session.hpp"

namespace xiinux {

using map_sessions = std::unordered_map<std::string, std::shared_ptr<session>>;

class sessions final {
  map_sessions sessions_{};

public:
  inline auto get(std::string_view id) -> std::shared_ptr<session> {
    return sessions_[std::string{id}];
  }

  inline void put(std::shared_ptr<session> ses) {
    sessions_[std::string{ses->get_id()}] = std::move(ses);
  }

} static sessions;
} // namespace xiinux
