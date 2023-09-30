// reviewed: 2023-09-27
#include "server.hpp"
#include <csignal>
#include <cstring>

[[noreturn]] static void sigint(int signum) {
  printf("caught signal: %s (%d)\nexiting\n", strsignal(signum), signum);
  xiinux::server::stop();
  exit(-signum);
}

int main(const int argc, const char *argv[]) {
  // catch all signals
  for (int i = 1; i < NSIG; i++) {
    signal(i, sigint);
  }
  // if SIGPIPE not ignored 'sendfile' aborts program when 'Broken pipe'
  signal(SIGPIPE, SIG_IGN);

  return xiinux::server::start(argc, argv);
}
