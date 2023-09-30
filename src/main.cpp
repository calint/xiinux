// reviewed: 2023-09-27
#include "server.hpp"
#include <csignal>
#include <cstring>

[[noreturn]] static void sigint(int signum) {
  printf("\n!!! caught signal %d: %s\n!!! exiting\n", signum,
         strsignal(signum));
  xiinux::server::stop();
  exit(-signum);
}

int main(const int argc, const char *argv[]) {
  // catch all signals
  for (int i = 1; i < NSIG; i++) {
    signal(i, sigint);
  }
  signal(SIGPIPE, SIG_IGN); // 'sendfile' raises signal when 'Broken pipe'
  return xiinux::server::start(argc, argv);
}
