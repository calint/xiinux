#pragma once
#include <exception>

namespace xiinux {
class connection_lost_exception final : public std::exception {};
} // namespace xiinux
