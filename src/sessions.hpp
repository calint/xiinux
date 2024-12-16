// reviewed: 2023-09-27
#pragma once
#include "session.hpp"

namespace xiinux {

using map_sessions = std::unordered_map<std::string, std::unique_ptr<session>>;

class sessions final {
  map_sessions sessions_{};

public:
  inline sessions() = default;
  sessions(const sessions &) = delete;
  auto operator=(const sessions &) -> sessions & = delete;
  sessions(sessions &&) = delete;
  auto operator=(sessions &&) -> sessions & = delete;

  inline ~sessions() {
    printf(" * deleting %zu session%s\n", sessions_.size(),
           sessions_.size() == 1 ? "" : "s");
  }

  inline auto get(const std::string_view &id) -> session * {
    return sessions_[std::string{id}].get();
  }

  inline void put(std::unique_ptr<session> ses) {
    sessions_[std::string{ses->get_id()}] = std::move(ses);
  }

} static sessions;

} // namespace xiinux
