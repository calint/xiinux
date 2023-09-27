#pragma once
#include <cstddef>

namespace xiinux::conf {
static constexpr const char *application_name = "xiinux web server";
static constexpr int epoll_event_array_size = 128;
static constexpr size_t sock_req_buf_size_in_bytes = K;
static constexpr size_t sock_content_buf_size_in_bytes = 4 * K;
static constexpr size_t chunky_buf_size_in_bytes = 4 * K;
static bool print_traffic = false;
static int print_traffic_fd = 1;
} // namespace xiinux::conf
