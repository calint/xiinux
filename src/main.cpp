// reviewed: 2023-09-27
#include "server.hpp"
#include "session.hpp"
#include <csignal>

inline void xiinux::session::put_widget(std::string path,
                                        std::unique_ptr<widget> wgt) {
    widgets_[std::move(path)] = std::move(wgt);
}

[[noreturn]] static void sigint(int signum) {
    printf("\n!!! caught signal %d: %s\n!!! exiting\n", signum,
           strsignal(signum)); // NOLINT(concurrency-mt-unsafe)
    xiinux::server::stop();
    exit(-signum); // NOLINT(concurrency-mt-unsafe)
}

auto main(const int argc, const char** argv) -> int {
    // catch all signals
    // for (int i = 1; i < NSIG; i++) {
    //   signal(i, sigint);
    // }
    (void)signal(SIGINT, sigint);  // close at '^C'
    (void)signal(SIGTERM, sigint); // 'killall' and 'kill' commands
    (void)signal(SIGPIPE,
                 SIG_IGN);     // 'sendfile' raises signal when 'Broken pipe'
    (void)signal(28, SIG_IGN); // 'Window changed'
    try {
        return xiinux::server::start(argc, argv);
    } catch (const std::exception& e) {
        printf("!!! exception in main: %s\n", e.what());
        return 1;
    }
}
