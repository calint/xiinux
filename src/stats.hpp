// reviewed: 2023-09-27
#pragma once
#include <cstdio>

namespace xiinux {

class stats final {
public:
  size_t ms = 0;
  size_t socks = 0;
  size_t sessions = 0;
  size_t requests = 0;
  size_t input = 0;
  size_t output = 0;
  size_t accepts = 0;
  size_t reads = 0;
  size_t writes = 0;
  size_t files = 0;
  size_t widgets = 0;
  size_t cache = 0;
  size_t errors = 0;
  size_t brkp = 0;

  static void print_headers(FILE *f) {
    (void)fprintf(f, "%12s%14s%14s%9s%10s%8s%8s%12s%12s%12s%12s%12s%8s%8s\n",
                  "ms", "input", "output", "socks", "reqs", "sesns", "accepts",
                  "reads", "writes", "files", "widgets", "cache", "errors",
                  "brkpipe");
    (void)fflush(f);
  }

  void print_stats(FILE *f) const {
    (void)fprintf(
        f, "%12zu%14zu%14zu%9zu%10zu%8zu%8zu%12zu%12zu%12zu%12zu%12zu%8zu%8zu",
        ms, input, output, socks, requests, sessions, accepts, reads, writes,
        files, widgets, cache, errors, brkp);
    (void)fflush(f);
  }
} static stats;

} // namespace xiinux
