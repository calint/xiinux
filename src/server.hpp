// reviewed: 2023-09-27
#pragma once
#include "args.hpp"
#include "defines.hpp"
#include "sock.hpp"
#include <netinet/tcp.h>

namespace xiinux {
class server final {
public:
  inline static int start(const int argc, const char **argv) {
    args a(argc, argv);
    thdwatch_on = a.has_option('m');
    const int port = atoi(a.get_option_value('p', "8088"));
    const bool option_benchmark_mode = a.has_option('b');
    conf::print_traffic = a.has_option('t');

    printf("%s on port %d", conf::application_name, port);
    if (option_benchmark_mode) {
      printf(" in benchmark mode");
    }
    printf("\n");

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

    if (listen(server_socket.fd_, conf::epoll_event_array_size) == -1) {
      perror("listen");
      exit(3);
    }

    epoll_fd = epoll_create(conf::epoll_event_array_size);
    if (epoll_fd == -1) {
      perror("epollcreate");
      exit(4);
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = &server_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket.fd_, &ev)) {
      perror("epolladd");
      exit(5);
    }

    init_homepage();

    if (thdwatch_on) {
      if (pthread_create(&thdwatch, nullptr, &thdwatch_run, nullptr)) {
        puts("pthread_create");
        exit(6);
      }
    }

    struct epoll_event events[conf::epoll_event_array_size];
    while (true) {
      const int n =
          epoll_wait(epoll_fd, events, conf::epoll_event_array_size, -1);
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
          const int fd = accept(server_socket.fd_, nullptr, nullptr);
          if (fd == -1) {
            perror("accept");
            continue;
          }
          int opts = fcntl(fd, F_GETFL);
          if (opts == -1) {
            perror("fncntl1");
            continue;
          }
          opts |= O_NONBLOCK;
          if (fcntl(fd, F_SETFL, opts) == -1) {
            perror("fncntl2");
            continue;
          }
          ev.data.ptr = new sock(fd);
          ev.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
          if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev)) {
            perror("epoll_ctl");
            continue;
          }

          // int bufsize = 8192;
          // setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(int));

          if (option_benchmark_mode) {
            int flag = 1;
            if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
                           static_cast<void *>(&flag), sizeof(int))) {
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
          if (msg == signal_connection_lost) {
            stats.brkp++;
            continue;
          }
          printf(" *** exception: %s\n", msg);
        } catch (...) {
          printf(" *** exception from %p\n", static_cast<void *>(c));
        }
      }
    }
  }

  inline static void stop() {
    delete homepage;
    thdwatch_on = false;
    pthread_join(thdwatch, nullptr);
  }

private:
  inline static void init_homepage() {
    char buf[4 * K];
    // +1 because of '\n' after 'application_name'
    const int res = snprintf(
        buf, sizeof(buf), "HTTP/1.1 200\r\nContent-Length: %zu\r\n\r\n%s\n",
        strlen(conf::application_name) + 1, conf::application_name);
    if (size_t(res) >= sizeof(buf)) {
      puts("homepage does not fit in buffer");
      exit(7);
    }
    homepage = new doc(buf);
  }

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
};
} // namespace xiinux
