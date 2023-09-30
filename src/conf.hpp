// reviewed: 2023-09-27
#pragma once
#include "defines.hpp"
#include <cstddef>

namespace xiinux::conf {
static constexpr const char *application_name = "xiinux web server";
static constexpr int server_event_array_size = 128;
static constexpr bool server_print_events = false;
static constexpr size_t sock_req_buf_size = K;
static constexpr size_t sock_content_buf_size = 4 * K;
static constexpr int sock_send_buffer_size = 0; // 0 means system default
static constexpr bool sock_print_getsock_len = false;
static constexpr size_t chunky_buf_size = 4 * K;
static constexpr size_t widget_key_size = 128;
static constexpr size_t upload_path_size = 256;
static constexpr unsigned path_to_widget_lut_size = 16;
static constexpr size_t str_len_max = K * M;

static bool print_traffic = false;
static int print_traffic_fd = 1;
} // namespace xiinux::conf
