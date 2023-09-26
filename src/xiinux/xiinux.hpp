#pragma once
#include "doc.hpp"
#include "widget.hpp"
namespace xiinux { // shared by server and sock to avoid circular ref
static int epollfd;
static doc *homepage;
static widget *widget_new(const char *qs);
} // namespace xiinux
