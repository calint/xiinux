#pragma once
#include <exception>

namespace xiinux {

class client_closed_exception final : public std::exception {};

class client_exception final : public std::exception {
  const char *msg_;

public:
  inline explicit client_exception(const char *msg) : msg_{msg} {}
  ~client_exception() override = default;
  client_exception(const client_exception &) = default;
  client_exception &operator=(const client_exception &) = default;
  client_exception(client_exception &&) = default;
  client_exception &operator=(client_exception &&) = default;

  inline const char *what() const noexcept override { return msg_; }
};

} // namespace xiinux
