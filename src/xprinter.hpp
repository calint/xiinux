// reviewed: 2023-09-27
#pragma once
#include "exceptions.hpp"
#include <array>
#include <cstdio>
#include <string_view>

namespace xiinux {

// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions,hicpp-special-member-functions)
class xprinter {

  public:
    virtual ~xprinter() = default;

    // virtuals

    virtual auto p(const std::string_view& sv) -> xprinter& = 0;

    virtual auto p(char ch) -> xprinter& = 0;

    inline virtual auto flush() -> xprinter& { return *this; }

    // functions using p(string_view) and p(char)

    inline auto p(const int i) -> xprinter& {
        std::array<char, array_size_nums> str{};
        const int len = snprintf(str.data(), str.size(), "%d", i);
        if (len < 0 or size_t(len) >= str.size()) {
            throw client_exception{"xprinter:2"};
        }
        return p({str.data(), size_t(len)});
    }

    inline auto p(const size_t sz) -> xprinter& {
        std::array<char, array_size_nums> str{};
        const int len = snprintf(str.data(), str.size(), "%zu", sz);
        if (len < 0 or size_t(len) >= str.size()) {
            throw client_exception{"xprinter:3"};
        }
        return p({str.data(), size_t(len)});
    }

    inline auto p_ptr(const void* ptr) -> xprinter& {
        std::array<char, array_size_nums> str{};
        const int len = snprintf(str.data(), str.size(), "%p", ptr);
        if (len < 0 or size_t(len) >= str.size()) {
            throw client_exception{"xprinter:4"};
        }
        return p({str.data(), size_t(len)});
    }

    inline auto p_hex(const int i) -> xprinter& {
        std::array<char, array_size_nums> str{};
        const int len = snprintf(str.data(), str.size(), "%x", unsigned(i));
        if (len < 0 or size_t(len) >= str.size()) {
            throw client_exception{"xprinter:5"};
        }
        return p({str.data(), size_t(len)});
    }

    inline auto nl() -> xprinter& { return p('\n'); }

  private:
    static constexpr size_t array_size_nums = 32;
};

} // namespace xiinux
