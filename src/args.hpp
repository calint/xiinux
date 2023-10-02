#pragma once

// todo: review messy but simple code
namespace xiinux {
class args final {
  const int argc_;
  const char **argv_;

public:
  inline args(const int argc, const char **argv) : argc_{argc}, argv_{argv} {}

  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  inline auto has_option(const char short_name) -> bool {
    if (argc_ == 1) {
      return false;
    }
    const char **argv = argv_;
    int argc = argc_;
    while (true) {
      if (argc == 1) {
        return false;
      }
      argv++;
      argc--;
      const char *p = *argv;
      if (*p == '-') {
        p++;
        while (true) {
          const char ch = *p;
          if (ch == short_name) {
            return true;
          }
          if (ch == '\0') {
            return false;
          }
          p++;
        }
      }
    }
  }

  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  inline auto get_option_value(const char short_name, const char *default_value)
      -> const char * {
    int argc = argc_ - 1;
    if (argc == 0) {
      return default_value;
    }
    const char **argv = argv_;
    while (true) {
      argv++;
      const char *p = *argv;
      if (*p == '-') {
        p++;
        while (true) {
          const char ch = *p;
          if (ch == '\0') {
            break;
          }
          if (ch == short_name) {
            p++;
            if (*p == '\0') { // e.g. "-p 8080"
              if (argc > 1) {
                return *(argv + 1);
              }
              return default_value;
            }
            return p; // e.g. "-p8080"
          }
          p++;
        }
      }
      argc--;
      if (argc == 0) {
        break;
      }
    }
    return default_value;
  }

  inline auto getarg(int n, const char *default_value) -> const char * {
    const char **argv = argv_;
    int argc = argc_;
    while (true) {
      if (argc == 1) {
        return default_value;
      }
      argv++;
      argc--;
      const char *p = *argv;
      if (*p == '-') {
        continue;
      }
      n--;
      if (n == 0) {
        return p;
      }
    }
  }
};
} // namespace xiinux
