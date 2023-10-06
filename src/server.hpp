// reviewed: 2023-09-27
//           2023-10-03
#pragma once
#include "args.hpp"
#include "defines.hpp"
#include "sock.hpp"
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <thread>

namespace xiinux {
class server final {
public:
  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  [[nodiscard]] inline static auto start(const int argc, const char **argv)
      -> int {
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
      return 1;
    }

    if constexpr (conf::server_tcp_fast_open_size) {
      // https://man7.org/linux/man-pages/man7/tcp.7.html
      // TCP_FASTOPEN (since Linux 3.6)
      // This option enables Fast Open (RFC 7413) on the listener
      // socket.  The value specifies the maximum length of pending
      // SYNs (similar to the backlog argument in listen(2)).  Once
      // enabled, the listener socket grants the TCP Fast Open
      // cookie on incoming SYN with TCP Fast Open option.
      int option = conf::server_tcp_fast_open_size;
      if (setsockopt(server_fd, IPPROTO_TCP, TCP_FASTOPEN, &option,
                     sizeof(option))) {
        perror("setsockopt TCP_FASTOPEN");
        return 2;
      }
    }

    if constexpr (conf::server_reuse_addr_and_port) {
      int option = 1;
      if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &option,
                     sizeof(option))) {
        perror("setsockopt SO_REUSEADDR");
        return 2;
      }
      if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &option,
                     sizeof(option))) {
        perror("setsockopt SO_REUSEPORT");
        return 3;
      }
    }

    if constexpr (conf::server_print_listen_socket_conf) {
      printf("SO_SNDBUF: %d\nTCP_NODELAY: %d\nTCP_CORK: %d\nTCP_FASTOPEN: %d\n",
             get_sock_option(server_fd, SO_SNDBUF, SOL_SOCKET),
             get_sock_option(server_fd, TCP_NODELAY, IPPROTO_TCP),
             get_sock_option(server_fd, TCP_CORK, IPPROTO_TCP),
             get_sock_option(server_fd, TCP_FASTOPEN, IPPROTO_TCP));
    }

    struct sockaddr_in server_addr {};
    const ssize_t server_addr_size = sizeof(server_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(uint16_t(port));

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto *server_sock_addr = reinterpret_cast<struct sockaddr *>(&server_addr);
    if (bind(server_fd, server_sock_addr, server_addr_size)) {
      perror("bind");
      return 4;
    }

    if (listen(server_fd, conf::server_listen_backlog_size) == -1) {
      perror("listen");
      return 5;
    }

    epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd == -1) {
      perror("epollcreate");
      return 6;
    }

    struct epoll_event server_ev {};
    server_ev.events = EPOLLIN;
    // address of server fd cannot be same as a client address
    server_ev.data.ptr = &server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &server_ev)) {
      perror("epolladd");
      return 7;
    }

    if (!init_homepage()) {
      puts("cannot initate homepage");
      return 8;
    }

    web::widget_init_path_to_factory_map();

    if (thdwatch_on) {
      thdwatch = std::jthread(thdwatch_run);
    }

    std::array<struct epoll_event, conf::server_listen_backlog_size> events{};
    while (true) {
      const int n = epoll_wait(epoll_fd, events.data(), events.size(), -1);
      if (n == -1) {
        if (errno == EINTR) {
          continue; // interrupted system call ok
        }
        perror("epoll_wait");
        continue;
      }

      if constexpr (conf::server_print_epoll_events) {
        printf("events %d\n", n);
      }

      for (unsigned i = 0; i < unsigned(n); i++) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        struct epoll_event &ev = events[i];
        // check if server socket
        if (ev.data.ptr == &server_fd) {
          // server, new connection
          stats.accepts++;

          struct sockaddr_in client_addr {};
          socklen_t client_addr_len = sizeof(client_addr);
          auto *client_sock_addr =
              // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
              reinterpret_cast<struct sockaddr *>(&client_addr);
          const int client_fd = accept4(server_fd, client_sock_addr,
                                        &client_addr_len, SOCK_NONBLOCK);
          if (client_fd == -1) {
            perror("accept");
            continue;
          }

          if constexpr (conf::server_print_client_connect_event) {
            std::array<char, INET_ADDRSTRLEN> ip_str_buf{};
            std::array<char, 26> time_str_buf{};
            printf("%s  %s  connect fd=%d\n", current_time_to_str(time_str_buf),
                   ip_addr_to_str(ip_str_buf, &(client_addr.sin_addr.s_addr)),
                   client_fd);
          }

          auto upc = std::make_unique<sock>(client_fd, client_addr);
          ev.data.ptr = upc.get();
          socks.insert({client_fd, std::move(upc)});
          ev.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
          if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev)) {
            perror("epoll_ctl");
            continue;
          }

          if constexpr (conf::sock_send_buffer_size) {
            if (setsockopt(client_fd, SOL_SOCKET, SO_SNDBUF,
                           &conf::sock_send_buffer_size,
                           sizeof(conf::sock_send_buffer_size))) {
              perror("setsockopt SO_SNDBUF");
            }
          }

          if (option_benchmark_mode) {
            int option = 1;
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

          if constexpr (conf::server_print_client_socket_conf) {
            printf("SO_SNDBUF: %d\nTCP_NODELAY: %d\nTCP_CORK: "
                   "%d\n",
                   get_sock_option(client_fd, SO_SNDBUF, SOL_SOCKET),
                   get_sock_option(client_fd, TCP_NODELAY, IPPROTO_TCP),
                   get_sock_option(client_fd, TCP_CORK, IPPROTO_TCP));
          }

          // run client right away without waiting for EPOLLIN
          // run_client(client);

          continue;
        }

        // client sock, read, write or hang-up available
        if constexpr (conf::server_print_epoll_events) {
          printf("client %p event=%x\n", ev.data.ptr, ev.events);
        }

        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
        sock *client = static_cast<sock *>(ev.data.ptr);

        if (ev.events & (EPOLLRDHUP | EPOLLHUP)) {
          socks.erase(client->get_fd());
          continue;
        }

        run_client(client);
      }
    }
  }

  inline static void stop() {
    printf("\n\nstopping xiinux\n");

    if (close(epoll_fd)) {
      perror("server:stop:close epoll_fd");
    }

    if (close(server_fd)) {
      perror("server:stop:close server_fd");
    }

    const size_t nsocks = socks.size();
    printf(" * disconnecting %lu socket%s\n", nsocks, nsocks == 1 ? "" : "s");
    socks.clear();

    if (thdwatch_on) {
      printf(" * send stop to metrics thread\n");
      thdwatch.request_stop();
    }
  }

private:
  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
  inline static void run_client(sock *client) {
    try {
      client->run();
    } catch (const client_closed_exception &) {
      stats.brkp++;
      socks.erase(client->get_fd());
    } catch (const client_exception &e) {
      stats.errors++;
      print_client_exception(client, e.what());
      socks.erase(client->get_fd());
    } catch (...) {
      stats.errors++;
      print_client_exception(client, "n/a due to catch(...)");
      socks.erase(client->get_fd());
    }
  }

  inline static void print_client_exception(const sock *client,
                                            const char *msg) {
    // client session id
    const std::string &ses_id = client->get_session_id();

    // client ip
    std::array<char, INET_ADDRSTRLEN> ip_addr_str{};
    in_addr_t addr = client->get_socket_address().sin_addr.s_addr;

    // current time
    std::array<char, 26> time_str_buf{};
    current_time_to_str(time_str_buf);

    // output
    printf(
        "!!! exception %s  %s  session=[%s]  path=[%s]  query=[%s]  msg=[%s]\n",
        time_str_buf.data(), ip_addr_to_str(ip_addr_str, &addr),
        ses_id.empty() ? "n/a" : ses_id.c_str(), client->get_path().data(),
        client->get_query().data(), msg);
  }

  [[nodiscard]] inline static auto init_homepage() -> bool {
    // +1 because of '\n' after 'application_name'
    const size_t content_len =
        strnlen(conf::application_name, conf::str_len_max) + 1;

    strb<4 * K> sb{};
    sb.p("HTTP/1.1 200\r\nContent-Length: "sv)
        .p(content_len)
        .p("\r\nContent-Type: text/plain\r\n\r\n")
        .p(conf::application_name)
        .p('\n');

    homepage = std::make_unique<doc>(sb.string());
    return true;
  }

  inline static std::jthread thdwatch{};
  inline static bool thdwatch_on = false;
  inline static bool thdwatch_stats_to_file = false;
  inline static void thdwatch_run(const std::stop_token &stoken) {
    stats::print_headers(stdout);
    constexpr int sleep_us = 100'000;
    constexpr int dt_ms = sleep_us / 1'000;
    constexpr bool metrics_print_new_line = true;
    while (!stoken.stop_requested()) {
      for (int i = 0; i < 10; i++) {
        usleep(sleep_us);
        stats.ms += dt_ms; //? not really
        stats.print_stats(stdout);
        printf(thdwatch_stats_to_file ? "\n" : "\r");
      }
      if (!thdwatch_stats_to_file and metrics_print_new_line) {
        (void)fprintf(stdout, "\n");
      }
    }
    printf(" * metrics watcher stopped\n");
  }

  inline static auto get_sock_option(const int fd, const int opt,
                                     const int protocol) -> int {
    int option = -1;
    socklen_t option_size = sizeof(option);
    if (getsockopt(fd, protocol, opt, &option, &option_size) == -1) {
      perror("get_sock_option");
      return -1;
    }
    return option;
  }

  inline static int server_fd{};
  inline static std::unordered_map<int, std::unique_ptr<sock>> socks{};
};
} // namespace xiinux
