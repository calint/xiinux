#pragma once
#include "doc.hpp"
#include "widget.hpp"

// shared by server and sock to avoid circular ref
namespace xiinux {
static int epoll_fd;
static doc *homepage;
static /*give*/ widget *widget_new(const char *qs);
} // namespace xiinux
