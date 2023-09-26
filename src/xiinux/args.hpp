#pragma once
#include <ctype.h>
// todo: review messy code
namespace xiinux {
class args final {
  const int argc_;
  const char **argv_;

public:
  inline args(const int argc, const char *argv[]) : argc_{argc}, argv_{argv} {}
  inline bool has_option(const char short_name) {
    if (argc_ == 1)
      return false;
    const char **vv = argv_;
    int i = argc_;
    while (true) {
      if (i == 1)
        return false;
      vv++;
      i--;
      const char *p = *vv;
      if (*p == '-') {
        p++;
        while (true) {
          const char ch = *p;
          if (ch == short_name)
            return true;
          if (ch == 0)
            return false;
          if (isdigit(ch))
            return false;
          p++;
        }
      }
    }
  }
  inline const char *get_option_value(const char short_name,
                                    const char *default_value) {
    int i = argc_ - 1;
    if (i == 0)
      return default_value;
    const char **vv = argv_;
    while (true) {
      vv++;
      const char *p = *vv;
      if (*p == '-') {
        p++;
        while (true) {
          const char ch = *p;
          if (!ch)
            break;
          if (ch == short_name) {
            p++;
            if (!*p) { //? secondparametervaluestartswith
              if (i > 1)
                return *(vv + 1);
              return default_value;
            }
            return p;
          }
          p++;
        }
      }
      i--;
      if (i == 0)
        break;
    }
    return default_value;
  }
  inline const char *getarg(int n, const char *default_value) {
    const char **vv = argv_;
    int i = argc_;
    while (true) {
      if (i == 1)
        return default_value;
      vv++;
      i--;
      const char *p = *vv;
      if (*p == '-')
        continue;
      n--;
      if (n == 0)
        return p;
    }
  }
};
} // namespace xiinux
