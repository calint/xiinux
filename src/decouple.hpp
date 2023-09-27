// reviewed: 2023-09-27
#pragma once
#include "doc.hpp"
#include "widget.hpp"

// shared by server and sock to avoid circular reference
// 'sock' uses 'epoll_fd', 'homepage' and 'widget_new'
// 'sock' does not refer to 'server'
// 'server' uses 'epoll_fd' and 'homepage'. refers to 'sock'
namespace xiinux {
static int epoll_fd;
static doc *homepage;
namespace web {
static /*give*/ widget *widget_new(const char *qs);
}
} // namespace xiinux
