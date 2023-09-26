#pragma once
#include "../web/web.hpp"
#include "args.hpp"
#include "conf.hpp"
#include "sessions.hpp"
#include "widget.hpp"
#include "xiinux.hpp"
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
namespace xiinux {
class sock final {
  enum state {
    method,
    uri,
    query,
    protocol,
    header_key,
    header_value,
    resume_send_file,
    receiving_content,
    receiving_upload,
    next_request
  } state = method;

  class file {
    off_t pos_ = 0;
    size_t len_ = 0;
    int fd_ = 0;

  public:
    inline void close() {
      if (::close(fd_) < 0)
        perror("closefile");
    }
    inline ssize_t resume_send_to(const int to_fd) {
      stats.writes++;
      const ssize_t n = sendfile(to_fd, fd_, &pos_, len_ - size_t(pos_));
      if (n < 0)
        return n;
      stats.output += size_t(n);
      return n;
    }
    inline void init_for_send(const size_t size_in_bytes,
                              const off_t seek_pos = 0) {
      pos_ = seek_pos;
      len_ = size_in_bytes;
    }
    inline bool done() const { return size_t(pos_) == len_; }
    inline size_t length() const { return len_; }
    inline int open(const char *path) {
      fd_ = ::open(path, O_RDONLY);
      return fd_;
    }
    inline void rst() {
      fd_ = 0;
      pos_ = 0;
      len_ = 0;
    }
  } file;

  struct reqline {
    char *pth_ = nullptr;
    char *qs_ = nullptr;
    inline void rst() { pth_ = qs_ = nullptr; }
  } reqline;

  struct header {
    char *key_ = nullptr;
    char *value_ = nullptr;
    inline void rst() { key_ = value_ = nullptr; }
  } header;

  class content {
    size_t pos_ = 0;
    size_t len_ = 0;
    char *buf_ = nullptr;

  public:
    inline void rst() { pos_ = len_ = 0; }
    inline void free() {
      if (!buf_)
        return;
      delete[] buf_;
      buf_ = nullptr;
    }
    inline bool more() const { return pos_ != len_; }
    inline size_t rem() const { return len_ - pos_; }
    inline void unsafe_skip(const size_t n) { pos_ += n; }

    inline size_t total_length() const { return len_; }
    inline void init_for_receive(const char *content_length_str) {
      pos_ = 0;
      len_ = size_t(atoll(content_length_str));
      if (!buf_) {
        buf_ = new char[sock_content_buf_size_in_bytes];
      }
    }
    inline char *ptr() const { return buf_; }
    inline ssize_t receive_from(int fd_in) {
      stats.reads++;
      const ssize_t n = recv(fd_in, buf_, sock_content_buf_size_in_bytes, 0);
      if (n < 0)
        return n;
      stats.input += size_t(n);
      if (conf::print_traffic) {
        const ssize_t m = write(conf::print_traffic_fd, buf_, size_t(n));
        if (m == -1 or m != n) {
          perror("reply:io_send");
        }
      }
      return n;
    }
  } content;

  class buf {
    char buf_[sock_req_buf_size_in_bytes];
    char *p_ = buf_;
    char *e_ = buf_;

  public:
    inline void rst() { p_ = e_ = buf_; }
    inline bool more() const { return p_ != e_; }
    inline size_t rem() const { return size_t(e_ - p_); }
    inline void unsafe_skip(const size_t n) { p_ += n; }
    inline char unsafe_next_char() { return *p_++; }
    inline void set_eos() { *(p_ - 1) = '\0'; }
    inline char *ptr() const { return p_; }
    inline ssize_t receive_from(const int fd_in) {
      const size_t nbytes_to_read =
          sock_req_buf_size_in_bytes - size_t(p_ - buf_);
      if (nbytes_to_read == 0)
        throw "sock:buf:full";
      stats.reads++;
      const ssize_t n = recv(fd_in, p_, nbytes_to_read, 0);
      if (n == -1)
        return n;
      stats.input += size_t(n);
      e_ = p_ + n;
      if (conf::print_traffic) {
        const ssize_t m = write(conf::print_traffic_fd, p_, size_t(n));
        if (m == -1 or m != n) {
          perror("incomplete or failed write");
        }
      }
      return n;
    }
  } buf;

  lut<const char *> hdrs_;
  int upload_fd_ = 0;
  widget *wdgt_ = nullptr;
  session *ses_ = nullptr;
  bool send_session_id_in_reply_ = false;

  inline void io_request_read() {
    struct epoll_event ev;
    ev.data.ptr = this;
    ev.events = EPOLLIN;
    if (epoll_ctl(epollfd, EPOLL_CTL_MOD, fd_, &ev))
      throw "sock:epollmodread";
  }

  inline void io_request_write() {
    struct epoll_event ev;
    ev.data.ptr = this;
    ev.events = EPOLLOUT;
    if (epoll_ctl(epollfd, EPOLL_CTL_MOD, fd_, &ev))
      throw "sock:epollmodwrite";
  }

  inline size_t io_send(const void *ptr, size_t len,
                        bool throw_if_send_not_complete = false) {
    stats.writes++;
    const ssize_t n = send(fd_, ptr, len, MSG_NOSIGNAL);
    if (n < 0) {
      if (errno == EPIPE or errno == ECONNRESET)
        throw signal_connection_reset_by_peer;
      stats.errors++;
      throw "sock:iosend";
    }
    const size_t nbytes_sent = size_t(n);
    stats.output += nbytes_sent;

    if (conf::print_traffic) {
      const ssize_t m = write(conf::print_traffic_fd, ptr, nbytes_sent);
      if (m == -1 or m != n) {
        perror("reply:io_send2");
      }
    }
    if (throw_if_send_not_complete and nbytes_sent != len) {
      stats.errors++;
      throw "sock:sendnotcomplete";
    }

    return nbytes_sent;
  }

public:
  int fd_ = 0;
  inline sock(const int f = 0) : fd_{f} { stats.socks++; }

  inline ~sock() {
    content.free();
    stats.socks--;
    if (!::close(fd_))
      return;
    stats.errors++;
    perr("sockdel");
  }

  inline void run() {
    while (true) {
      if (state == resume_send_file) {
        const ssize_t n = file.resume_send_to(fd_);
        if (n < 0) { // error or send buffer full
          if (errno == EAGAIN) {
            io_request_write();
            return;
          }
          if (errno == EPIPE or errno == ECONNRESET)
            throw signal_connection_reset_by_peer;
          stats.errors++;
          throw "sock:err2";
        }
        if (!file.done())
          continue;
        file.close();
        state = next_request;
      } else if (state == receiving_content) {
        const ssize_t n = content.receive_from(fd_);
        if (n == 0)
          throw signal_connection_reset_by_peer;
        if (n < 0) {
          if (errno == EAGAIN or errno == EWOULDBLOCK) {
            io_request_read();
            return;
          } else if (errno == ECONNRESET)
            throw signal_connection_reset_by_peer;
          stats.errors++;
          throw "sock:receiving_content";
        }
        const size_t un = size_t(n);
        const size_t crem = content.rem();
        const size_t total = content.total_length();
        reply x{fd_};
        if (crem > un) { // not finished
          wdgt_->on_content(x, content.ptr(), un, total);
          content.unsafe_skip(un);
          continue;
        }
        // last chunk
        wdgt_->on_content(x, content.ptr(), crem, total);
        state = next_request;
      } else if (state == receiving_upload) {
        const ssize_t n = content.receive_from(fd_);
        if (n == 0)
          throw signal_connection_reset_by_peer;
        if (n < 0) {
          if (errno == EAGAIN or errno == EWOULDBLOCK) {
            io_request_read();
            return;
          } else if (errno == ECONNRESET)
            throw signal_connection_reset_by_peer;
          stats.errors++;
          throw "sock:receiving_upload";
        }
        const size_t un = size_t(n);
        const size_t rem = content.rem();
        const size_t nbytes_to_write = rem > un ? un : rem;
        const ssize_t m = write(upload_fd_, content.ptr(), nbytes_to_write);
        if (m == -1 or size_t(m) != nbytes_to_write) {
          stats.errors++;
          perror("sock:run:upload");
          throw "sock:writing upload to file";
        }
        if (m != n)
          throw "sock:writing upload to file 2";
        content.unsafe_skip(unsigned(m));
        if (content.more())
          continue;
        if (::close(upload_fd_) < 0)
          perr("while closing upload file 2");
        io_send("HTTP/1.1 204\r\n\r\n", 16, true); // 16 is the length of string
        state = next_request;
      }
      if (state == next_request) {
        // if previous request had header 'Connection: close'
        const char *connection = hdrs_["connection"];
        if (connection and !strcmp("close", connection)) {
          delete this;
          return;
        }
        stats.requests++;
        file.rst();
        reqline.rst();
        hdrs_.clear();
        header.rst();
        content.rst();
        upload_fd_ = 0;
        wdgt_ = nullptr;
        ses_ = nullptr;
        buf.rst();
        state = method;
      }
      if (!buf.more()) {
        const ssize_t n = buf.receive_from(fd_);
        if (n == 0) { // closed by client
          delete this;
          return;
        }
        if (n == -1) { // error or would block
          if (errno == EAGAIN or errno == EWOULDBLOCK) {
            io_request_read();
            return;
          } else if (errno == ECONNRESET) {
            throw signal_connection_reset_by_peer;
          }
          perror("sock:run:io_request_read");
          stats.errors++;
          throw "sock:err3";
        }
      }
      if (state == method) {
        while (buf.more()) {
          const char c = buf.unsafe_next_char();
          if (c == ' ') {
            state = uri;
            reqline.pth_ = buf.ptr();
            break;
          }
        }
      }
      if (state == uri) {
        while (buf.more()) {
          const char c = buf.unsafe_next_char();
          if (c == ' ') {
            buf.set_eos();
            state = protocol;
            break;
          } else if (c == '?') {
            buf.set_eos();
            reqline.qs_ = buf.ptr();
            state = query;
            break;
          }
        }
      }
      if (state == query) {
        while (buf.more()) {
          const char c = buf.unsafe_next_char();
          if (c == ' ') {
            buf.set_eos();
            state = protocol;
            break;
          }
        }
      }
      if (state == protocol) {
        while (buf.more()) {
          const char c = buf.unsafe_next_char();
          if (c == '\n') {
            header.key_ = buf.ptr();
            state = header_key;
            break;
          }
        }
      }
      if (state == header_key) {
        while (buf.more()) {
          const char c = buf.unsafe_next_char();
          if (c == '\n') { // content or done parsing
            do_after_headers();
            break;
          } else if (c == ':') {
            buf.set_eos();
            header.value_ = buf.ptr();
            state = header_value;
            break;
          }
        }
      }
      if (state == header_value) {
        while (buf.more()) {
          const char c = buf.unsafe_next_char();
          if (c == '\n') {
            buf.set_eos();
            // -2 to skip '\0' and place pointer on last character in the key
            header.key_ = strtrm(header.key_, header.value_ - 2);
            // RFC 2616: header field names are case-insensitive
            strlwr(header.key_);
            //? -2 to skip '\0' and place pointer on last character in the value
            header.value_ = strtrm(header.value_, buf.ptr() - 2);
            // printf("%s: %s\n",hdrparser.key,hdrparser.value);
            hdrs_.put(header.key_, header.value_);
            header.key_ = buf.ptr();
            state = header_key;
            break;
          }
        }
      }
    }
  }

private:
  void do_serve_widget() {
    stats.widgets++;
    const char *cookie = hdrs_["cookie"];
    const char *session_id = nullptr;
    if (cookie and strstr(cookie, "i=")) {
      // -1 to exclude '\0'
      session_id = cookie + sizeof("i=") - 1;
    }
    if (!session_id) {
      // create session
      time_t timer = time(nullptr);
      struct tm *tm_info = gmtime(&timer);
      char *sid = new char[24];
      // 20150411-225519-ieu44dn
      strftime(sid, size_t(24), "%Y%m%d-%H%M%S-", tm_info);
      // 16 is len of "20150411-225519-"
      char *sid_ptr = sid + 16;
      for (unsigned i = 0; i < 7; i++) {
        *sid_ptr++ = 'a' + char((random()) % 26);
      }
      *sid_ptr = '\0';
      ses_ = new session(/*give*/ sid);
      sessions.put(ses_, false);
      send_session_id_in_reply_ = true;
    } else {
      ses_ = sessions.get(session_id);
      if (!ses_) {
        // 24 is the size of session id including '\0'
        // e.g: "20150411-225519-ieu44dn\0"
        char *sid = new char[24];
        strncpy(sid, session_id, 23);
        // make sure sid is terminated
        sid[23] = '\0';
        ses_ = new session(/*give*/ sid);
        sessions.put(/*give*/ ses_, false);
      }
    }
    //? remake to bind path to widget factories
    wdgt_ = ses_->get_widget(reqline.qs_);
    if (!wdgt_) {
      wdgt_ = widget_new(reqline.qs_);
      const size_t key_len = strnlen(reqline.qs_, 1024);
      if (key_len == 1024)
        throw "sock:key_len";
      // +1 for the \0 terminator
      char *key = new char[key_len + 1];
      memcpy(key, reqline.qs_, key_len + 1);
      ses_->put_widget(/*give*/ key, /*give*/ wdgt_);
    }
    reply x{fd_};
    if (send_session_id_in_reply_) {
      x.send_session_id_at_next_opportunity(ses_->id());
      send_session_id_in_reply_ = false;
    }
    const size_t total_content_len = content.total_length();
    if (total_content_len) { // posting content to widget
      wdgt_->on_content(x, nullptr, 0, total_content_len);
      // if client expects 100 continue before sending post
      const char *s = hdrs_["expect"];
      if (s and !strcmp(s, "100-continue")) {
        io_send("HTTP/1.1 100\r\n\r\n", 16, true);
        state = receiving_content;
        return;
      }
      const size_t rem = buf.rem();
      if (rem >= total_content_len) { // full content is in 'buf'
        wdgt_->on_content(x, buf.ptr(), total_content_len, total_content_len);
        state = next_request;
        return;
      } else {
        // part of the content is in 'buf'
        wdgt_->on_content(x, buf.ptr(), rem, total_content_len);
        content.unsafe_skip(rem);
        state = receiving_content;
        return;
      }
    }
    // requesting widget
    wdgt_->to(x);
    state = next_request;
  }

  void do_serve_upload() {
    // file upload
    char bf[256];
    // +1 to skip the leading '/'
    if (snprintf(bf, sizeof(bf), "upload/%s", reqline.pth_ + 1) == sizeof(bf))
      throw "sock:pathtrunc";
    if ((upload_fd_ = open(bf, O_CREAT | O_WRONLY | O_TRUNC, 0664)) < 0) {
      perror("while creating file for upload");
      throw "sock:err7";
    }
    // check if client expects 100-continue before sending content
    const char *s = hdrs_["expect"];
    if (s and !strcmp(s, "100-continue")) {
      // 16 is string length
      io_send("HTTP/1.1 100\r\n\r\n", 16, true);
      state = receiving_upload;
      return;
    }
    const size_t rem = buf.rem();
    if (rem == 0) {
      state = receiving_upload;
      return;
    }
    const size_t total = content.total_length();
    if (rem >= total) {
      // the whole file is in 'buf'
      const ssize_t n = write(upload_fd_, buf.ptr(), total);
      if (n == -1 or size_t(n) != total) {
        perror("could not write");
        throw "sock:err4";
      }
      if (size_t(n) != total) {
        throw "sock:incomplete upload";
      }
      if (::close(upload_fd_) < 0) {
        perr("while closing upload file");
      }
      const char resp[] = "HTTP/1.1 204\r\n\r\n";
      // -1 to exclude '\0'
      io_send(resp, sizeof(resp) - 1, true);
      state = next_request;
      return;
    }
    // part of the file is in 'buf'
    const ssize_t n = write(upload_fd_, buf.ptr(), rem);
    if (n == 1 or size_t(n) != rem) {
      perror("while writing upload to file2");
      throw "sock:err6";
    }
    if (size_t(n) != rem) {
      throw "upload2";
    }
    content.unsafe_skip(size_t(n));
    state = receiving_upload;
  }

  void do_serve_file(reply &x, const char *path) {
    if (strstr(path, "..")) {
      constexpr char err[] = "path contains ..\n";
      // -1 to not include '\0'
      x.http(403, err, sizeof(err) - 1);
      state = next_request;
      return;
    }
    struct stat fdstat;
    if (stat(path, &fdstat)) {
      constexpr char err[] = "not found\n";
      // -1 to not include '\0'
      x.http(404, err, sizeof(err) - 1);
      state = next_request;
      return;
    }
    if (S_ISDIR(fdstat.st_mode)) {
      constexpr char err[] = "path is directory\n";
      // -1 to not include '\0'
      x.http(403, err, sizeof(err) - 1);
      state = next_request;
      return;
    }
    const struct tm *tm = gmtime(&fdstat.st_mtime);
    char lastmod[64];
    // example: "Fri, 31 Dec 1999 23:59:59 GMT"
    if (!strftime(lastmod, sizeof(lastmod), "%a, %d %b %y %H:%M:%S %Z", tm)) {
      throw "sock:strftime";
    }
    const char *lastmodstr = hdrs_["if-modified-since"];
    if (lastmodstr and !strcmp(lastmodstr, lastmod)) {
      constexpr char hdr[] = "HTTP/1.1 304\r\n\r\n";
      // -1 to not include '\0'
      io_send(hdr, sizeof(hdr) - 1, true);
      state = next_request;
      return;
    }
    if (file.open(path) < 0) {
      constexpr char err[] = "cannot open\n";
      // -1 to not include '\0'
      x.http(404, err, sizeof(err) - 1);
      state = next_request;
      return;
    }
    stats.files++;
    const char *range = hdrs_["range"];
    char header_buf[512];
    int header_buf_len;
    if (range and *range) {
      off_t rs = 0;
      if (sscanf(range, "bytes=%jd", &rs) == EOF) { //? is sscanf safe
        stats.errors++;
        perr("range");
        throw "sock:errrorscanning";
      }
      file.init_for_send(size_t(fdstat.st_size), rs);
      const size_t e = file.length();
      header_buf_len = snprintf(header_buf, sizeof(header_buf),
                                "HTTP/1.1 206\r\nAccept-Ranges: "
                                "bytes\r\nLast-Modified: %s\r\nContent-Length: "
                                "%zu\r\nContent-Range: %zu-%zu/%zu\r\n\r\n",
                                lastmod, e - size_t(rs), rs, e, e);
    } else {
      file.init_for_send(size_t(fdstat.st_size));
      header_buf_len =
          snprintf(header_buf, sizeof(header_buf),
                   "HTTP/1.1 200\r\nAccept-Ranges: bytes\r\nLast-Modified: "
                   "%s\r\nContent-Length: %zu\r\n\r\n",
                   lastmod, file.length());
    }
    if (header_buf_len == sizeof(header_buf) or header_buf_len < 0)
      throw "sock:err1";
    io_send(header_buf, size_t(header_buf_len), true);

    const ssize_t nn = file.resume_send_to(fd_);
    if (nn < 0) {
      if (errno == EPIPE or errno == ECONNRESET)
        throw signal_connection_reset_by_peer;
      stats.errors++;
      perr("sendingfile");
      throw "sock:err5";
    }
    if (!file.done()) {
      state = resume_send_file;
      return;
    }
    file.close();
    state = next_request;
  }

  void do_after_headers() {
    const char *content_length_str = hdrs_["content-length"];
    if (content_length_str) {
      content.init_for_receive(content_length_str);
    }
    const char *path = *reqline.pth_ == '/' ? reqline.pth_ + 1 : reqline.pth_;
    // printf("path: '%s'\nquery: '%s'\n",path,reqline.qs);
    if (!*path and reqline.qs_) {
      do_serve_widget();
      return;
    }
    const char *content_type = hdrs_["content-type"];
    if (content_type and strstr(content_type, "file")) {
      do_serve_upload();
      return;
    }
    reply x{fd_};
    if (!*path) { // uri '/'
      stats.cache++;
      homepage->to(x);
      state = next_request;
      return;
    }
    do_serve_file(x, path);
  }

  static inline char *strtrm(char *p, char *e) {
    while (p != e and isspace(*p))
      p++;
    while (p != e and isspace(*e))
      *e-- = 0;
    return p;
  }
  static inline void strlwr(char *p) {
    while (*p) {
      *p = char(tolower(*p));
      p++;
    }
  }
} static server_socket;
} // namespace xiinux
