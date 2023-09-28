// reviewed: 2023-09-27
#pragma once
#include <cstddef>

namespace xiinux::conf {
static constexpr const char *application_name = "xiinux web server";
static constexpr int epoll_event_array_size = 128;
static constexpr size_t sock_req_buf_size = K;
static constexpr size_t sock_content_buf_size = 4 * K;
static constexpr size_t chunky_buf_size = 4 * K;
static constexpr size_t widget_key_size = 128;
static constexpr size_t upload_path_size = 256;
static constexpr size_t str_len_max = K * M;
static bool print_traffic = false;
static int print_traffic_fd = 1;
} // namespace xiinux::conf
