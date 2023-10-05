// reviewed: 2023-09-27
#pragma once

namespace xiinux::conf {
static constexpr const char *application_name = "xiinux web server";
static constexpr int server_listen_backlog_size = 128;
static constexpr bool server_reuse_addr_and_port = true;
static constexpr bool server_tcp_fast_open = true;
static constexpr bool server_print_listen_socket_conf = true;
static constexpr bool server_print_client_socket_conf = false;
static constexpr bool server_print_client_connect_event = false;
static constexpr bool server_print_client_disconnect_event = false;
static constexpr bool server_print_epoll_events = false;
static constexpr bool sock_print_client_requests = false;
static constexpr size_t sock_request_header_buf_size = K;
static constexpr size_t sock_content_buf_size = 4 * K;
static constexpr int sock_send_buffer_size = 0; // 0 means system default
static constexpr size_t sock_response_header_buffer_size = 512;
static constexpr size_t chunky_buf_size = 4 * K;
static constexpr size_t widget_key_size = 128;
static constexpr size_t upload_path_size = 256;
static constexpr unsigned path_to_widget_lut_size = 16;
static constexpr size_t str_len_max = K * M;
static constexpr bool print_traffic = false;
static constexpr int print_traffic_fd = 1; // stdout
} // namespace xiinux::conf
