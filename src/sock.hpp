// reviewed: 2023-09-27
#pragma once
#include "args.hpp"
#include "conf.hpp"
#include "decouple.hpp"
#include "doc.hpp"
#include "sessions.hpp"
#include "web/web.hpp"
#include "widget.hpp"
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/stat.h>

namespace xiinux {
class sock final {
public:
  int fd_ = 0;
  inline sock(const int f = 0) : fd_{f} {
    stats.socks++;
    // printf("client create %p\n", static_cast<void *>(this));
  }
  inline sock(const sock &) = delete;
  inline sock &operator=(const sock &) = delete;

  inline ~sock() {
    content.free();
    if (!::close(fd_)) {
      // printf("client close %p\n", static_cast<void *>(this));
      stats.socks--;
      return;
    }
    // note: epoll removes entry when all descriptions of fd_ are closed
    //       not needed: epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd_, nullptr);
    stats.errors++;
    perror("sock:destructor");
  }

  inline void run() {
    while (true) {
      if (state == resume_send_file) {
        const ssize_t n = file.resume_send_to(fd_);
        if (n == -1) { // error or send buffer full
          if (errno == EAGAIN) {
            io_request_write();
            return;
          }
          if (errno == EPIPE or errno == ECONNRESET)
            throw signal_connection_lost;
          stats.errors++;
          throw "sock:err2";
        }
        if (!file.is_done())
          continue;
        file.close();
        state = next_request;
      } else if (state == receiving_content) {
        const ssize_t n = content.receive_from(fd_);
        if (n == -1) {
          if (errno == EAGAIN) {
            io_request_read();
            return;
          } else if (errno == ECONNRESET)
            throw signal_connection_lost;
          stats.errors++;
          throw "sock:receiving_content";
        }
        const size_t nbytes_read = size_t(n);
        const size_t content_rem = content.remaining();
        const size_t content_len = content.content_len();
        reply x{fd_};
        widget_->on_content(x, content.buf(), nbytes_read,
                            content.pos() + nbytes_read, content_len);
        if (content_rem > nbytes_read) { // not finished
          content.unsafe_skip(nbytes_read);
          continue;
        }
        // this was the last part
        state = next_request;
      } else if (state == receiving_upload) {
        const ssize_t n = content.receive_from(fd_);
        if (n == -1) {
          if (errno == EAGAIN) {
            io_request_read();
            return;
          } else if (errno == ECONNRESET)
            throw signal_connection_lost;
          stats.errors++;
          throw "sock:receiving_upload";
        }
        const size_t nbytes_read = size_t(n);
        const size_t upload_rem = content.remaining();
        const size_t nbytes_to_write =
            upload_rem > nbytes_read ? nbytes_read : upload_rem;
        const ssize_t m = write(upload_fd_, content.buf(), nbytes_to_write);
        if (m == -1 or size_t(m) != nbytes_to_write) {
          stats.errors++;
          perror("sock:run:upload");
          throw "sock:writing upload to file 1";
        }
        if (m != n)
          throw "sock:writing upload to file 2";
        content.unsafe_skip(size_t(m));
        if (content.remaining())
          continue;
        if (::close(upload_fd_)) {
          perror("sock:closing upload file");
        }
        constexpr const char msg[] = "HTTP/1.1 204\r\n\r\n";
        // -1 to not include '\0'
        io_send(fd_, msg, sizeof(msg) - 1);
        state = next_request;
      }
      if (state == next_request) {
        // if previous request had header 'Connection: close'
        const char *connection = headers_["connection"];
        if (connection and !strcmp("close", connection)) {
          delete this;
          return;
        }
        stats.requests++;
        file.rst();
        reqline.rst();
        buf.rst();
        header.rst();
        content.rst();
        headers_.clear();
        upload_fd_ = 0;
        widget_ = nullptr;
        session_ = nullptr;
        state = method;
      }
      if (!buf.has_more()) {
        const ssize_t n = buf.receive_from(fd_);
        if (n == -1) { // error or would block
          if (errno == EAGAIN) {
            io_request_read();
            return;
          } else if (errno == ECONNRESET) {
            throw signal_connection_lost;
          }
          perror("sock:run:io_request_read");
          stats.errors++;
          throw "sock:err3";
        }
      }
      if (state == method) {
        while (buf.has_more()) {
          const char ch = buf.unsafe_next_char();
          if (ch == ' ') {
            state = uri;
            reqline.path_ = buf.ptr();
            break;
          }
        }
      }
      if (state == uri) {
        while (buf.has_more()) {
          const char ch = buf.unsafe_next_char();
          if (ch == ' ') {
            buf.set_eos();
            state = protocol;
            break;
          } else if (ch == '?') {
            buf.set_eos();
            reqline.query_str_ = buf.ptr();
            state = query;
            break;
          }
        }
      }
      if (state == query) {
        while (buf.has_more()) {
          const char ch = buf.unsafe_next_char();
          if (ch == ' ') {
            buf.set_eos();
            state = protocol;
            break;
          }
        }
      }
      if (state == protocol) {
        while (buf.has_more()) {
          const char ch = buf.unsafe_next_char();
          if (ch == '\n') {
            header.name_ = buf.ptr();
            state = header_key;
            break;
          }
        }
      }
      if (state == header_key) {
        while (buf.has_more()) {
          const char ch = buf.unsafe_next_char();
          if (ch == '\n') { // content or done parsing
            do_after_headers();
            break;
          } else if (ch == ':') {
            buf.set_eos();
            header.value_ = buf.ptr();
            state = header_value;
            break;
          }
        }
      }
      if (state == header_value) {
        while (buf.has_more()) {
          const char c = buf.unsafe_next_char();
          if (c == '\n') {
            buf.set_eos();
            // -2 to skip '\0' and place pointer on last character in the key
            header.name_ = strtrm(header.name_, header.value_ - 2);
            // RFC 2616: header field names are case-insensitive
            strlwr(header.name_);
            // -2 to skip '\0' and place pointer on last character in the value
            header.value_ = strtrm(header.value_, buf.ptr() - 2);
            // printf("%s: %s\n",hdrparser.key,hdrparser.value);
            headers_.put(header.name_, header.value_);
            header.name_ = buf.ptr();
            state = header_key;
            break;
          }
        }
      }
    }
  }

private:
  void do_after_headers() {
    widget *(*factory)() = web::widget_factory_for_path(reqline.path_);
    if (factory) {
      do_serve_widget(factory);
      return;
    }

    const char *content_type = headers_["content-type"];
    if (content_type and strstr(content_type, "file")) {
      content.init_for_receive(headers_["content-length"]);
      do_serve_upload();
      return;
    }

    reply x{fd_};

    const char *path =
        *reqline.path_ == '/' ? reqline.path_ + 1 : reqline.path_;
    if (!*path) { // uri '/'
      stats.cache++;
      homepage->to(x);
      state = next_request;
      return;
    }

    do_serve_file(x, path);
  }

  void do_serve_widget(widget *(*factory)()) {
    stats.widgets++;

    retrieve_or_create_session();

    widget_ = session_->get_widget(reqline.path_);
    if (!widget_) {
      widget_ = /*take*/ factory();
      const size_t key_len = strnlen(reqline.path_, conf::widget_key_size);
      if (key_len == conf::widget_key_size)
        throw "sock:key_len";
      // +1 for the \0 terminator
      char *key = new char[key_len + 1];
      memcpy(key, reqline.path_, key_len + 1);
      session_->put_widget(/*give*/ key, /*give*/ widget_);
    }
    reply x{fd_};
    if (send_session_id_in_reply_) {
      x.send_session_id_at_next_opportunity(session_->id());
      send_session_id_in_reply_ = false;
    }
    const char *content_length_str = headers_["content-length"];
    if (!content_length_str) {
      // no content from client, render widget
      widget_->to(x);
      state = next_request;
      return;
    }

    // content from client
    content.init_for_receive(content_length_str);
    const size_t content_len = content.content_len();
    // initiating call to widget with buf=nullptr
    widget_->on_content(x, nullptr, 0, 0, content_len);
    // if client expects 100 continue before sending post
    const char *expect = headers_["expect"];
    if (expect and !strcmp(expect, "100-continue")) {
      constexpr const char msg[] = "HTTP/1.1 100\r\n\r\n";
      // -1 to not include '\0'
      io_send(fd_, msg, sizeof(msg) - 1);
      state = receiving_content;
      return;
    }
    const size_t rem = buf.remaining();
    if (rem >= content_len) { // full content is in 'buf'
      widget_->on_content(x, buf.ptr(), content_len, content_len, content_len);
      state = next_request;
      return;
    }
    // beginning of the content is in 'buf'
    widget_->on_content(x, buf.ptr(), rem, rem, content_len);
    content.unsafe_skip(rem);
    state = receiving_content;
  }

  void retrieve_or_create_session() {
    const char *cookie = headers_["cookie"];
    const char *session_id = nullptr;
    if (cookie and strstr(cookie, "i=")) {
      // -1 to exclude '\0'
      session_id = cookie + sizeof("i=") - 1;
    }
    if (!session_id) {
      // no session id, create session
      time_t timer = time(nullptr);
      struct tm *tm_info = gmtime(&timer);
      // format to e.g. '20150411-225519-ieu44dn'
      char *sid = new char[24];
      if (!strftime(sid, size_t(24), "%Y%m%d-%H%M%S-", tm_info)) {
        throw "sock:do_serve_widget:1";
      }
      // 16 is len of "20150411-225519-"
      char *sid_ptr = sid + 16;
      for (unsigned i = 0; i < 7; i++) {
        *sid_ptr++ = 'a' + char((random()) % 26);
      }
      *sid_ptr = '\0';
      session_ = new session(/*give*/ sid);
      sessions.put(session_, false);
      send_session_id_in_reply_ = true;
      return;
    }

    // try to get active session
    session_ = sessions.get(session_id);
    if (session_)
      return;

    // session not found, create
    // 24 is the size of session id including '\0'
    // e.g: "20150411-225519-ieu44dn\0"
    char *sid = new char[24];
    strncpy(sid, session_id, 23);
    // make sure sid is terminated
    sid[23] = '\0';
    session_ = new session(/*give*/ sid);
    sessions.put(/*give*/ session_, false);
  }

  void do_serve_upload() {
    // file upload
    char pth[conf::upload_path_size];
    // +1 to skip the leading '/'
    const int res = snprintf(pth, sizeof(pth), "upload/%s", reqline.path_ + 1);
    if (res < 0 or size_t(res) >= sizeof(pth))
      throw "sock:pathtrunc";
    upload_fd_ = open(pth, O_CREAT | O_WRONLY | O_TRUNC, 0664);
    if (upload_fd_ == -1) {
      perror("while creating file for upload");
      throw "sock:err7";
    }
    // check if client expects 100-continue before sending content
    const char *expect = headers_["expect"];
    if (expect and !strcmp(expect, "100-continue")) {
      constexpr const char msg[] = "HTTP/1.1 100\r\n\r\n";
      // -1 to not include '\0'
      io_send(fd_, msg, sizeof(msg) - 1);
      state = receiving_upload;
      return;
    }
    const size_t remaining = buf.remaining();
    if (remaining == 0) {
      state = receiving_upload;
      return;
    }
    const size_t content_len = content.content_len();
    if (remaining >= content_len) {
      // the whole file is in 'buf'
      const ssize_t n = write(upload_fd_, buf.ptr(), content_len);
      if (n == -1 or size_t(n) != content_len) {
        perror("could not write");
        throw "sock:err4";
      }
      if (size_t(n) != content_len) {
        throw "sock:incomplete upload";
      }
      if (::close(upload_fd_)) {
        perr("while closing upload file");
      }
      constexpr const char msg[] = "HTTP/1.1 204\r\n\r\n";
      // -1 to not include '\0'
      io_send(fd_, msg, sizeof(msg) - 1);
      state = next_request;
      return;
    }
    // start of the file is in 'buf'
    const ssize_t n = write(upload_fd_, buf.ptr(), remaining);
    if (n == -1 or size_t(n) != remaining) {
      perror("while writing upload to file2");
      throw "sock:err6";
    }
    content.unsafe_skip(size_t(n));
    state = receiving_upload;
  }

  void do_serve_file(reply &x, const char *path) {
    if (strstr(path, "..")) {
      constexpr char msg[] = "path contains ..\n";
      // -1 to not include '\0'
      x.http(403, msg, sizeof(msg) - 1);
      state = next_request;
      return;
    }
    struct stat fdstat;
    if (stat(path, &fdstat)) {
      constexpr char msg[] = "not found\n";
      // -1 to not include '\0'
      x.http(404, msg, sizeof(msg) - 1);
      state = next_request;
      return;
    }
    if (S_ISDIR(fdstat.st_mode)) {
      constexpr char msg[] = "path is directory\n";
      // -1 to not include '\0'
      x.http(403, msg, sizeof(msg) - 1);
      state = next_request;
      return;
    }
    const struct tm *tm = gmtime(&fdstat.st_mtime);
    char lastmod[64];
    // e.g.: 'Fri, 31 Dec 1999 23:59:59 GMT'
    if (!strftime(lastmod, sizeof(lastmod), "%a, %d %b %y %H:%M:%S %Z", tm)) {
      throw "sock:strftime";
    }
    const char *lastmodstr = headers_["if-modified-since"];
    if (lastmodstr and !strcmp(lastmodstr, lastmod)) {
      constexpr char msg[] = "HTTP/1.1 304\r\n\r\n";
      // -1 to not include '\0'
      io_send(fd_, msg, sizeof(msg) - 1);
      state = next_request;
      return;
    }
    if (file.open(path) == -1) {
      constexpr char msg[] = "cannot open file\n";
      // -1 to not include '\0'
      x.http(404, msg, sizeof(msg) - 1);
      state = next_request;
      return;
    }
    stats.files++;
    const char *range = headers_["range"];
    char header_buf[512];
    int header_buf_len = 0;
    if (range and *range) {
      off_t offset = 0;
      if (sscanf(range, "bytes=%jd", &offset) == EOF) {
        stats.errors++;
        perr("range");
        throw "sock:errrorscanning";
      }
      file.init_for_send(size_t(fdstat.st_size), offset);
      const size_t len = file.length();
      header_buf_len =
          snprintf(header_buf, sizeof(header_buf),
                   "HTTP/1.1 206\r\nAccept-Ranges: "
                   "bytes\r\nLast-Modified: %s\r\nContent-Length: "
                   "%zu\r\nContent-Range: %zu-%zu/%zu\r\n\r\n",
                   lastmod, len - size_t(offset), offset, len, len);
    } else {
      file.init_for_send(size_t(fdstat.st_size));
      header_buf_len =
          snprintf(header_buf, sizeof(header_buf),
                   "HTTP/1.1 200\r\nAccept-Ranges: bytes\r\nLast-Modified: "
                   "%s\r\nContent-Length: %zu\r\n\r\n",
                   lastmod, file.length());
    }
    if (header_buf_len < 0 or size_t(header_buf_len) >= sizeof(header_buf))
      throw "sock:err1";
    io_send(fd_, header_buf, size_t(header_buf_len), true);

    const ssize_t n = file.resume_send_to(fd_);
    if (n == -1) {
      if (errno == EPIPE or errno == ECONNRESET)
        throw signal_connection_lost;
      stats.errors++;
      perr("sendingfile");
      throw "sock:err5";
    }
    if (!file.is_done()) {
      state = resume_send_file;
      return;
    }
    file.close();
    state = next_request;
  }

  static inline char *strtrm(char *p, char *e) {
    while (p != e and isspace(*p))
      p++;
    while (p != e and isspace(*e))
      *e-- = '\0';
    return p;
  }

  static inline void strlwr(char *p) {
    while (*p) {
      *p = char(tolower(*p));
      p++;
    }
  }

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
    off_t offset_ = 0;
    size_t count_ = 0;
    int fd_ = 0;

  public:
    inline void close() {
      if (::close(fd_)) {
        perror("closefile");
      }
    }
    inline ssize_t resume_send_to(const int out_fd) {
      stats.writes++;
      const ssize_t n =
          sendfile(out_fd, fd_, &offset_, count_ - size_t(offset_));
      if (n == -1) // error
        return n;
      stats.output += size_t(n);
      return n;
    }
    inline void init_for_send(const size_t size_in_bytes,
                              const off_t seek_pos = 0) {
      offset_ = seek_pos;
      count_ = size_in_bytes;
    }
    inline bool is_done() const { return size_t(offset_) == count_; }
    inline size_t length() const { return count_; }
    inline int open(const char *path) {
      fd_ = ::open(path, O_RDONLY);
      return fd_;
    }
    inline void rst() {
      fd_ = 0;
      offset_ = 0;
      count_ = 0;
    }
  } file{};

  struct reqline {
    char *path_ = nullptr;
    char *query_str_ = nullptr;
    inline void rst() { path_ = query_str_ = nullptr; }
  } reqline{};

  struct header {
    char *name_ = nullptr;
    char *value_ = nullptr;
    inline void rst() { name_ = value_ = nullptr; }
  } header{};

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
    inline char *buf() const { return buf_; }
    inline size_t pos() const { return pos_; }
    inline size_t remaining() const { return len_ - pos_; }
    inline void unsafe_skip(const size_t n) { pos_ += n; }
    inline size_t content_len() const { return len_; }

    inline void init_for_receive(const char *content_length_str) {
      pos_ = 0;
      len_ = size_t(atoll(content_length_str));
      // todo: abuse len
      // todo: atoll error
      if (!buf_) {
        buf_ = new char[conf::sock_content_buf_size];
      }
    }

    inline ssize_t receive_from(int fd_in) {
      stats.reads++;
      const ssize_t n = recv(fd_in, buf_, conf::sock_content_buf_size, 0);
      if (n == -1) // error
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
  } content{};

  class buf {
    char buf_[conf::sock_req_buf_size];
    char *p_ = buf_;
    char *e_ = buf_;

  public:
    inline void rst() { p_ = e_ = buf_; }
    inline char *ptr() const { return p_; }
    inline bool has_more() const { return p_ != e_; }
    inline size_t remaining() const { return size_t(e_ - p_); }
    inline void unsafe_skip(const size_t n) { p_ += n; }
    inline char unsafe_next_char() { return *p_++; }
    inline void set_eos() { *(p_ - 1) = '\0'; }

    inline ssize_t receive_from(const int fd_in) {
      const size_t nbytes_to_read = conf::sock_req_buf_size - size_t(p_ - buf_);
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
  } buf{};

  lut<const char *> headers_{};
  int upload_fd_ = 0;
  widget *widget_ = nullptr;
  session *session_ = nullptr;
  bool send_session_id_in_reply_ = false;

  inline void io_request_read() {
    struct epoll_event ev;
    // note. not necessary to assign 'ev.data.fd' because it is only used to
    // compare with 'server_fd'
    ev.data.ptr = this;
    ev.events = EPOLLIN | EPOLLRDHUP;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd_, &ev))
      throw "sock:epollmodread";
  }

  inline void io_request_write() {
    struct epoll_event ev;
    // see note in io_request_read
    ev.data.ptr = this;
    ev.events = EPOLLOUT | EPOLLRDHUP;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd_, &ev))
      throw "sock:epollmodwrite";
  }
};
} // namespace xiinux
