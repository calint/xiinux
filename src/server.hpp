// reviewed: 2023-09-27
#pragma once
#include "args.hpp"
#include "defines.hpp"
#include "sock.hpp"
#include <arpa/inet.h>
#include <iomanip>
#include <iostream>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <thread>

namespace xiinux {
class server final {
public:
  inline static int start(const int argc, const char *argv[]) {
    args a(argc, argv);
    thdwatch_on = a.has_option('m');
    thdwatch_stats_to_file = a.has_option('f');
    const int port = atoi(a.get_option_value('p', "8088"));
    const bool option_benchmark_mode = a.has_option('b');

    printf("%s on port %d", conf::application_name, port);
    if (option_benchmark_mode) {
      printf(" in benchmark mode");
    }
    printf("\n");

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
      perror("socket");
      exit(1);
    }

    int option = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &option,
                   sizeof(option))) {
      perror("setsockopt SO_REUSEADDR");
      exit(1);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &option,
                   sizeof(option))) {
      perror("setsockopt SO_REUSEPORT");
      exit(2);
    }

    if (conf::server_print_listen_socket_conf) {
      printf("SO_SNDBUF: %d\nTCP_NODELAY: %d\nTCP_CORK: %d\n",
             get_sock_option(server_fd, SO_SNDBUF, SOL_SOCKET),
             get_sock_option(server_fd, TCP_NODELAY),
             get_sock_option(server_fd, TCP_CORK));
    }

    struct sockaddr_in server_addr;
    const ssize_t server_addr_size = sizeof(server_addr);
    bzero(&server_addr, server_addr_size);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(uint16_t(port));

    if (bind(server_fd, reinterpret_cast<struct sockaddr *>(&server_addr),
             server_addr_size)) {
      perror("bind");
      exit(2);
    }

    if (listen(server_fd, conf::server_event_array_size) == -1) {
      perror("listen");
      exit(3);
    }

    epoll_fd = epoll_create(conf::server_event_array_size);
    if (epoll_fd == -1) {
      perror("epollcreate");
      exit(4);
    }

    struct epoll_event server_ev;
    server_ev.events = EPOLLIN;
    // address of server fd cannot be same as a client address
    server_ev.data.ptr = &server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &server_ev)) {
      perror("epolladd");
      exit(5);
    }

    init_homepage();

    web::widget_init_path_to_factory_map();

    if (thdwatch_on) {
      thdwatch = std::thread(thdwatch_run);
    }

    struct epoll_event events[conf::server_event_array_size];
    while (true) {
      const int n =
          epoll_wait(epoll_fd, events, conf::server_event_array_size, -1);
      if (n == -1) {
        if (errno == EINTR)
          continue; // interrupted system call ok
        perror("epoll_wait");
        continue;
      }
      if (conf::server_print_events) {
        printf("events %d\n", n);
      }
      for (unsigned i = 0; i < unsigned(n); i++) {
        struct epoll_event &ev = events[i];
        // check if server socket
        if (ev.data.ptr == &server_fd) {
          // server, new connection
          stats.accepts++;

          struct sockaddr_in client_addr;
          socklen_t client_addr_len;
          const int client_fd = accept4(
              server_fd, reinterpret_cast<struct sockaddr *>(&client_addr),
              &client_addr_len, SOCK_NONBLOCK);
          if (client_fd == -1) {
            perror("accept");
            continue;
          }

          if (conf::server_print_events) {
            printf("client connect: event=%x fd=%d ip=%s\n", ev.events,
                   client_fd, inet_ntoa(client_addr.sin_addr));
          }

          sock *client = new sock(client_fd, client_addr);
          ev.data.ptr = client;
          ev.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
          if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev)) {
            perror("epoll_ctl");
            continue;
          }

          if (conf::sock_send_buffer_size) {
            if (setsockopt(client_fd, SOL_SOCKET, SO_SNDBUF,
                           &conf::sock_send_buffer_size,
                           sizeof(conf::sock_send_buffer_size))) {
              perror("setsockopt SO_SNDBUF");
            }
          }

          if (option_benchmark_mode) {
            // note. setsockopt may fail to set value without raising error
            option = 1;
            if (setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &option,
                           sizeof(option))) {
              perror("setsockopt TCP_NODELAY");
            }
            option = 0;
            if (setsockopt(client_fd, IPPROTO_TCP, TCP_CORK, &option,
                           sizeof(option))) {
              perror("setsockopt TCP_CORK");
            }
          }

          if (conf::server_print_client_socket_conf) {
            printf("SO_SNDBUF: %d\nTCP_NODELAY: %d\nTCP_CORK: %d\n",
                   get_sock_option(client_fd, SO_SNDBUF, SOL_SOCKET),
                   get_sock_option(client_fd, TCP_NODELAY),
                   get_sock_option(client_fd, TCP_CORK));
          }

          // run client right away without waiting for EPOLLIN
          // run_client(client);

          continue;
        }

        // client sock, read, write or hang-up available
        if (conf::server_print_events) {
          printf("client %p event=%x\n", ev.data.ptr, ev.events);
        }

        sock *client = static_cast<sock *>(ev.data.ptr);

        if (ev.events & (EPOLLRDHUP | EPOLLHUP)) {
          delete client;
          continue;
        }

        run_client(client);
      }
    }
  }

  inline static void stop() {
    if (close(epoll_fd)) {
      perror("server:stop:close epoll_fd");
    }
    if (close(server_fd)) {
      perror("server:stop:close server_fd");
    }
    if (thdwatch_on) {
      thdwatch_on = false;
      thdwatch.join();
    }
  }

private:
  inline static void run_client(sock *client) {
    try {
      client->run();
    } catch (const char *msg) {
      if (msg == signal_connection_lost) {
        stats.brkp++;
        delete client;
        return;
      }
      stats.errors++;
      print_client_exception(client, msg);
      delete client;
    } catch (...) {
      stats.errors++;
      print_client_exception(client, "n/a due to catch(...)");
      delete client;
    }
  }

  inline static void print_client_exception(const sock *client,
                                            const char *msg) {
    const session *sn = client->get_session();
    const char *snid = sn ? sn->get_id() : "n/a";

    const std::time_t t = std::time(nullptr);
    const std::tm tm = *std::localtime(&t);
    std::cout << "!!! exception " << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
              << "  " << inet_ntoa(client->get_socket_address().sin_addr)
              << "  session=" << snid << "  msg=" << msg << std::endl;
  }

  inline static void init_homepage() {
    char buf[4 * K];
    // +1 because of '\n' after 'application_name'
    const int n =
        snprintf(buf, sizeof(buf),
                 "HTTP/1.1 200\r\nContent-Length: %zu\r\nContent-Type: "
                 "text/plain\r\n\r\n%s\n",
                 strnlen(conf::application_name, conf::str_len_max) + 1,
                 conf::application_name);
    if (n < 0 or size_t(n) >= sizeof(buf)) {
      puts("homepage does not fit in buffer");
      exit(7);
    }
    homepage = std::make_unique<doc>(buf, size_t(n));
  }

  inline static std::thread thdwatch{};
  inline static bool thdwatch_on = false;
  inline static bool thdwatch_stats_to_file = false;
  inline static void thdwatch_run() {
    stats.print_headers(stdout);
    while (thdwatch_on) {
      int n = 10;
      while (thdwatch_on and n--) {
        const int sleep_us = 100'000;
        usleep(sleep_us);
        stats.ms += sleep_us / 1'000; //? not really
        stats.print_stats(stdout);
        printf(thdwatch_stats_to_file ? "\n" : "\r");
      }
      if (!thdwatch_stats_to_file) {
        fprintf(stdout, "\n");
      }
    }
  }

  inline static int get_sock_option(const int fd, const int opt,
                                    const int protocol = IPPROTO_TCP) {
    int option = -1;
    socklen_t option_size = sizeof(option);
    if (getsockopt(fd, protocol, opt, &option, &option_size) == -1) {
      perror("get_sock_option");
      return -1;
    }
    return option;
  }

  inline static int server_fd = 0;
};
} // namespace xiinux
