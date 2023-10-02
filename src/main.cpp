// reviewed: 2023-09-27
#include "server.hpp"
#include <cctype>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <memory>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>

[[noreturn]] static void sigint(int signum) {
  printf("\n!!! caught signal %d: %s\n!!! exiting\n", signum,
         strsignal(signum)); // NOLINT(concurrency-mt-unsafe)
  xiinux::server::stop();
  exit(-signum); // NOLINT(concurrency-mt-unsafe)
}

auto main(const int argc, const char **argv) -> int {
  // catch all signals
  // for (int i = 1; i < NSIG; i++) {
  //   signal(i, sigint);
  // }
  (void)signal(SIGINT, sigint);   // close at '^C'
  (void)signal(SIGPIPE, SIG_IGN); // 'sendfile' raises signal when 'Broken pipe'
  (void)signal(28, SIG_IGN);      // 'Window changed'
  return xiinux::server::start(argc, argv);
}
