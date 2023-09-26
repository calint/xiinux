#pragma once
#include <stdio.h>
namespace xiinux {
static constexpr int K = 1024;
static constexpr int M = K * K;
static constexpr const char *signal_connection_reset_by_peer = "brk";

#define perr(str)                                                              \
  do {                                                                         \
    printf("%s:%d %s", __FILE__, __LINE__, __PRETTY_FUNCTION__);               \
    perror(str); /*throw"perr";*/                                              \
  } while (false)

#define dbg(str)                                                               \
  do {                                                                         \
    printf("%s:%d %s\n  %s\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, str);  \
  } while (false)
} // namespace xiinux
