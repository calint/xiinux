#pragma once
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <execinfo.h>
#include <regex>

namespace xiinux {

class client_closed_exception final : public std::exception {};

class client_exception final : public std::exception {
  const char *msg_;

public:
  inline explicit client_exception(const char *msg) : msg_{msg} {
    // NOLINTBEGIN
    const int max_frames = 20;
    void *frames[max_frames];
    int num_frames = backtrace(frames, max_frames);
    char **symbols = backtrace_symbols(frames, num_frames);
    puts("----------------------------------------------");
    printf("exception: %s\n", msg);
    std::regex pattern(R"(xiinux\(\+(0x[0-9a-fA-F]+)\))");
    // 1 to skip the line that invoked backtrace(...)
    std::string command = "addr2line -C -f -i -p -s -e xiinux ";
    for (int i = 1; i < num_frames; ++i) {
      std::smatch matches;
      auto line = std::string(symbols[i]);
      if (std::regex_search(line, matches, pattern)) {
        if (matches.size() > 1) {
          std::string address = matches[1].str();
          command += address;
          command += " ";
        }
      }
    }
    free(symbols);
    FILE *pipe = popen(command.c_str(), "r");
    if (!pipe) {
      printf("!!! error opening pipe to addr2line\n");
      return;
    }
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
      printf("    %s", buffer);
    }
    pclose(pipe);
    // NOLINTEND
  }

  ~client_exception() override = default;
  client_exception(const client_exception &) = default;
  auto operator=(const client_exception &) -> client_exception & = default;
  client_exception(client_exception &&) = default;
  auto operator=(client_exception &&) -> client_exception & = default;

  [[nodiscard]] inline auto what() const noexcept -> const char * override {
    return msg_;
  }
};

} // namespace xiinux
