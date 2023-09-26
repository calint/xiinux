#pragma once
#include "args.hpp"
#include "defines.hpp"
#include "sock.hpp"
#include <netinet/tcp.h>
namespace xiinux {
class server final {
  inline static pthread_t thdwatch{};
  inline static bool thdwatch_on = false;
  inline static void *thdwatch_run(void *arg) {
    stats.print_headers(stdout);
    while (thdwatch_on) {
      int n = 10;
      while (thdwatch_on and n--) {
        const int sleep_us = 100'000;
        usleep(sleep_us);
        stats.ms += sleep_us / 1'000; //? not really
        stats.print_stats(stdout);
      }
      fprintf(stdout, "\n");
    }
    return nullptr;
  }

public:
  inline static void stop() {
    delete homepage;
    thdwatch_on = false;
    pthread_join(thdwatch, nullptr);
  }

  inline static int start(const int argc, const char **argv) {
    args a(argc, argv);
    thdwatch_on = a.has_option('v');
    const int port = atoi(a.get_option_value('p', "8088"));
    const bool option_benchmark_mode = a.has_option('b');
    conf::print_traffic = a.has_option('t');
    printf("%s on port %d\n", application_name, port);

    char buf[4 * K];
    // +1 because of '\n' after 'application_name'
    //? check return value
    snprintf(buf, sizeof(buf),
             "HTTP/1.1 200\r\nContent-Length: %zu\r\n\r\n%s\n",
             strlen(application_name) + 1, application_name);
    homepage = new doc(buf);

    struct sockaddr_in sa;
    const ssize_t sa_sz = sizeof(sa);
    bzero(&sa, sa_sz);
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = INADDR_ANY;
    sa.sin_port = htons(port);
    server_socket.fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket.fd_ == -1) {
      perror("socket");
      exit(1);
    }
    if (bind(server_socket.fd_, reinterpret_cast<struct sockaddr *>(&sa),
             sa_sz)) {
      perror("bind");
      exit(2);
    }
    if (listen(server_socket.fd_, nclients) == -1) {
      perror("listen");
      exit(3);
    }
    epollfd = epoll_create(nclients);
    if (!epollfd) {
      perror("epollcreate");
      exit(4);
    }
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = &server_socket;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, server_socket.fd_, &ev) < 0) {
      perror("epolladd");
      exit(5);
    }
    struct epoll_event events[nclients];
    if (thdwatch_on) {
      if (pthread_create(&thdwatch, nullptr, &thdwatch_run, nullptr)) {
        puts("pthread_create");
        exit(6);
      }
    }
    while (true) {
      const int n = epoll_wait(epollfd, events, nclients, -1);
      if (n == -1) {
        if (errno == EINTR)
          continue; // interrupted system call ok
        perror("epoll_wait");
        continue;
      }
      for (unsigned i = 0; i < unsigned(n); i++) {
        sock *c = static_cast<sock *>(events[i].data.ptr);
        // check if server socket
        if (c->fd_ == server_socket.fd_) {
          // new connection
          stats.accepts++;
          const int fda = accept(server_socket.fd_, nullptr, nullptr);
          if (fda == -1) {
            perror("accept");
            continue;
          }
          int opts = fcntl(fda, F_GETFL);
          if (opts < 0) {
            perror("fncntl1");
            continue;
          }
          opts |= O_NONBLOCK;
          if (fcntl(fda, F_SETFL, opts)) {
            perror("fncntl2");
            continue;
          }
          ev.data.ptr = new sock(fda);
          ev.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
          if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fda, &ev)) {
            perror("epoll_ctl");
            continue;
          }
          if (option_benchmark_mode) {
            int flag = 1;
            if (setsockopt(fda, IPPROTO_TCP, TCP_NODELAY,
                           static_cast<void *>(&flag), sizeof(int)) < 0) {
              perror("setsockopt TCP_NODELAY");
              continue;
            }
          }
          continue;
        }
        // read or write available
        try {
          c->run();
        } catch (const char *msg) {
          // todo: print session id, ip
          delete c;
          if (msg == signal_connection_reset_by_peer) {
            stats.brkp++;
            continue;
          }
          //					printf(" *** exception from %p :
          //%s\n",(void*)c,msg);
          printf(" *** exception: %s\n", msg);
        } catch (...) {
          printf(" *** exception from %p\n", static_cast<void *>(c));
        }
      }
    }
  }
};
} // namespace xiinux
