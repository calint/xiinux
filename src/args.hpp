#pragma once
#include <ctype.h>

// todo: review messy but simple code
namespace xiinux {
class args final {
  const int argc_;
  const char **argv_;

public:
  inline args(const int argc, const char *argv[]) : argc_{argc}, argv_{argv} {}

  inline bool has_option(const char short_name) {
    if (argc_ == 1)
      return false;
    const char **argv = argv_;
    int argc = argc_;
    while (true) {
      if (argc == 1)
        return false;
      argv++;
      argc--;
      const char *p = *argv;
      if (*p == '-') {
        p++;
        while (true) {
          const char ch = *p;
          if (ch == short_name)
            return true;
          if (!ch)
            return false;
          p++;
        }
      }
    }
  }

  inline const char *get_option_value(const char short_name,
                                      const char *default_value) {
    int argc = argc_ - 1;
    if (!argc)
      return default_value;
    const char **argv = argv_;
    while (true) {
      argv++;
      const char *p = *argv;
      if (*p == '-') {
        p++;
        while (true) {
          const char ch = *p;
          if (!ch)
            break;
          if (ch == short_name) {
            p++;
            if (!*p) { // e.g. "-p 8080"
              if (argc > 1)
                return *(argv + 1);
              return default_value;
            }
            return p; // e.g. "-p8080"
          }
          p++;
        }
      }
      argc--;
      if (!argc)
        break;
    }
    return default_value;
  }
  inline const char *getarg(int n, const char *default_value) {
    const char **argv = argv_;
    int argc = argc_;
    while (true) {
      if (argc == 1)
        return default_value;
      argv++;
      argc--;
      const char *p = *argv;
      if (*p == '-')
        continue;
      n--;
      if (!n)
        return p;
    }
  }
};
} // namespace xiinux
