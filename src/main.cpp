#include "server.hpp"
#include <signal.h>
[[noreturn]] static void sigint(int i) {
  puts("exiting");
  xiinux::server::stop();
  exit(0);
}
int main(const int c, const char **a) {
  signal(SIGINT, sigint);
  // if SIGPIPE not ignored 'sendfile' aborts program when 'Broken pipe'
  signal(SIGPIPE, SIG_IGN);
  return xiinux::server::start(c, a);
}
