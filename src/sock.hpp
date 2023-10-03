// reviewed: 2023-09-27
//           2023-10-03
#pragma once
#include "conf.hpp"
#include "decouple.hpp"
#include "doc.hpp"
#include "sessions.hpp"
#include "web/web.hpp"
#include <fcntl.h>
#include <memory>
#include <netinet/in.h>
#include <string_view>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace xiinux {

class sock final {
public:
  inline sock(const int fd, struct sockaddr_in sock_addr)
      : fd_{fd}, sock_addr_{sock_addr} {
    stats.socks++;
    // printf("client create %p\n", static_cast<void *>(this));
  }
  sock(const sock &) = delete;
  auto operator=(const sock &) -> sock & = delete;
  sock(sock &&) = delete;
  auto operator=(sock &&) -> sock & = delete;

  inline ~sock() {
    stats.socks--;
    if constexpr (conf::server_print_client_disconnect_event) {
      std::array<char, INET_ADDRSTRLEN> ip_str_buf{};
      std::array<char, 26> time_str_buf{};
      printf("%s  %s  disconnect fd=%d\n", current_time_to_str(time_str_buf),
             ip_addr_to_str(ip_str_buf, &(sock_addr_.sin_addr.s_addr)), fd_);
    }
    if (!::close(fd_)) {
      // printf("client close %p\n", static_cast<void *>(this));
      return;
    }
    // note.
    // epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd_, nullptr);
    // not necessary since epoll removes the entry when the file descriptor is
    // closed. (man epoll -> see questions and answers)
    stats.errors++;
    perror("sock:destructor");
  }

  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  inline void run() {
    while (true) {
      if (state_ == resume_send_file) {
        const ssize_t n = file_.resume_send_to(fd_);
        if (n == -1) { // error or send buffer full
          if (errno == EAGAIN) {
            io_request_write();
            return;
          }
          if (errno == EPIPE or errno == ECONNRESET) {
            throw client_closed_exception{};
          }
          stats.errors++;
          throw client_exception{"sock:err2"};
        }
        if (!file_.is_done()) {
          continue;
        }
        file_.close();
        state_ = next_request;
      } else if (state_ == receiving_content) {
        const ssize_t n = content_.receive_from(fd_);
        if (n == -1) {
          if (errno == EAGAIN) {
            io_request_read();
            return;
          }
          if (errno == ECONNRESET) {
            throw client_closed_exception{};
          }
          stats.errors++;
          throw client_exception{"sock:receiving_content"};
        }
        const auto nbytes_read = size_t(n);
        const size_t content_rem = content_.remaining();
        const size_t content_len = content_.content_len();
        reply x{fd_, reqline_.path_, reqline_.query_, headers_,
                &session_->get_lut()};
        widget_->on_content(x, content_.buf().data(), nbytes_read,
                            content_.pos() + nbytes_read, content_len);
        if (content_rem > nbytes_read) {
          // not finished
          content_.unsafe_skip(nbytes_read);
          continue;
        }
        // this was the last part
        state_ = next_request;
      } else if (state_ == receiving_upload) {
        const ssize_t n = content_.receive_from(fd_);
        if (n == -1) {
          if (errno == EAGAIN) {
            io_request_read();
            return;
          }
          if (errno == ECONNRESET) {
            throw client_closed_exception{};
          }
          stats.errors++;
          throw client_exception{"sock:receiving_upload"};
        }
        const auto nbytes_read = size_t(n);
        const size_t upload_rem = content_.remaining();
        const size_t nbytes_to_write =
            upload_rem > nbytes_read ? nbytes_read : upload_rem;
        const ssize_t m =
            write(upload_fd_, content_.buf().data(), nbytes_to_write);
        if (m == -1 or size_t(m) != nbytes_to_write) {
          stats.errors++;
          perror("sock:run:upload");
          throw client_exception{"sock:writing upload to file 1"};
        }
        if (m != n) {
          throw client_exception{"sock:writing upload to file 2"};
        }
        content_.unsafe_skip(size_t(m));
        if (content_.remaining()) {
          continue;
        }
        if (::close(upload_fd_)) {
          perror("sock:closing upload file");
        }
        io_send(fd_, "HTTP/1.1 204\r\n\r\n"sv);
        state_ = next_request;
      }
      if (state_ == next_request) {
        // note. int http 1.1 default is to keep connections open
        // if previous request had header 'Connection: close'
        // auto connection = headers_["connection"];
        // if (connection == "close") {
        //   printf("*** close\n"); // todo
        //   return;
        // }
        stats.requests++;
        file_.rst();
        reqline_.rst();
        reqbuf_.rst();
        header_.rst();
        content_.rst();
        headers_.clear();
        upload_fd_ = 0;
        widget_ = nullptr;
        session_id_ = {};
        session_ = nullptr;
        state_ = method;
      }
      if (!reqbuf_.has_more()) {
        const ssize_t n = reqbuf_.receive_from(fd_);
        if (n == -1) { // error or would block
          if (errno == EAGAIN) {
            io_request_read();
            return;
          }
          if (errno == ECONNRESET) {
            throw client_closed_exception{};
          }
          perror("sock:run:io_request_read");
          stats.errors++;
          throw client_exception{"sock:err3"};
        }
      }
      if (state_ == method) {
        while (reqbuf_.has_more()) {
          const char ch = reqbuf_.unsafe_next_char();
          if (ch == ' ') {
            state_ = uri;
            reqbuf_.set_mark();
            break;
          }
        }
      }
      if (state_ == uri) {
        while (reqbuf_.has_more()) {
          const char ch = reqbuf_.unsafe_next_char();
          if (ch == ' ') {
            reqbuf_.set_eos();
            reqline_.path_ = reqbuf_.string_view_from_mark();
            state_ = protocol;
            break;
          }
          if (ch == '?') {
            reqbuf_.set_eos();
            reqline_.path_ = reqbuf_.string_view_from_mark();
            state_ = query;
            break;
          }
        }
      }
      if (state_ == query) {
        while (reqbuf_.has_more()) {
          const char ch = reqbuf_.unsafe_next_char();
          if (ch == ' ') {
            reqbuf_.set_eos();
            reqline_.query_ = reqbuf_.string_view_from_mark();
            state_ = protocol;
            break;
          }
        }
      }
      if (state_ == protocol) {
        while (reqbuf_.has_more()) {
          const char ch = reqbuf_.unsafe_next_char();
          if (ch == '\n') {
            reqbuf_.set_mark();
            state_ = header_key;
            break;
          }
        }
      }
      if (state_ == header_key) {
        while (reqbuf_.has_more()) {
          const char ch = reqbuf_.unsafe_next_char();
          if (ch == '\n') { // content or done parsing
            do_after_headers();
            break;
          }
          if (ch == ':') {
            reqbuf_.set_eos();
            // RFC 2616: header field names are case-insensitive
            strlwr(reqbuf_.get_mark()); // to lower string
            header_.name_ = reqbuf_.string_view_from_mark();
            header_.name_ = trim(header_.name_);
            state_ = header_value;
            break;
          }
        }
      }
      if (state_ == header_value) {
        while (reqbuf_.has_more()) {
          const char c = reqbuf_.unsafe_next_char();
          if (c == '\n') {
            reqbuf_.set_eos();
            header_.value_ = reqbuf_.string_view_from_mark();
            header_.value_ = trim(header_.value_);

            // std::cout << "key :'" << header_.name_ << "' value '"
            //           << header_.value_ << "'\n";

            // headers_.insert({std::string_view{header_.name_},
            //                  std::string_view{header_.value_}});

            // headers_.insert({{header_.name_}, {header_.value_}});

            // headers_.insert({header_.name_, header_.value_});

            // headers_[{header_.name_}] = {header_.value_};

            headers_[header_.name_] = header_.value_;
            state_ = header_key;
            break;
          }
        }
      }
    }
  }

  inline auto get_socket_address() const -> const struct sockaddr_in & {
    return sock_addr_;
  }

  inline auto get_fd() const -> int { return fd_; }
  inline auto get_path() const -> std::string_view { return reqline_.path_; }
  inline auto get_query() const -> std::string_view { return reqline_.query_; }
  inline auto get_headers() const -> const map_headers & { return headers_; }
  inline auto get_session() const -> session * { return session_; }
  inline auto get_session_id() const -> const std::string & {
    return session_id_;
  }

private:
  void do_after_headers() {
    retrieve_session_id_from_cookie();
    if constexpr (conf::sock_print_client_requests) {
      // client ip
      std::array<char, INET_ADDRSTRLEN> ip_addr_str{};
      in_addr_t addr = get_socket_address().sin_addr.s_addr;

      // current time
      std::array<char, 26> time_str_buf{};
      current_time_to_str(time_str_buf);

      // output
      printf("%s  %s  session=%s  path=%s  query=%s\n", time_str_buf.data(),
             ip_addr_to_str(ip_addr_str, &addr),
             session_id_.empty() ? "n/a" : session_id_.c_str(),
             get_path().data(), get_query().data());
    }

    // check if there is a widget factory bound to path
    const widget_factory_func_ptr factory =
        web::widget_factory_for_path(reqline_.path_);
    if (factory) {
      // this path is served by a widget
      do_serve_widget(factory);
      return;
    }

    // check if request is an upload
    const std::string_view content_type = headers_["content-type"];
    if (content_type == "file") {
      // this is an upload, initiate content receive
      content_.init_for_receive(headers_["content-length"]);
      do_serve_upload();
      return;
    }

    // create reply
    reply x{fd_, reqline_.path_, reqline_.query_, headers_, nullptr};

    // remove leading '/' from path if exists
    const std::string_view path =
        reqline_.path_.at(0) == '/' ? reqline_.path_.substr(1) : reqline_.path_;
    // check if it is root path, uri '/'
    if (path.empty()) {
      // send 'homepage'
      stats.cache++;
      homepage->to(x);
      state_ = next_request;
      return;
    }
    // path refers to file
    do_serve_file(x, path);
  }

  void do_serve_widget(widget_factory_func_ptr factory) {
    stats.widgets++;

    retrieve_session_id_from_cookie();

    retrieve_or_create_session();

    // get widget from session using path
    widget_ = session_->get_widget(reqline_.path_);
    if (!widget_) {
      // widget not found in session, create using supplied factory
      auto up_wdgt = std::unique_ptr<widget>(factory());
      widget_ = up_wdgt.get();
      // move widget to session
      session_->put_widget(std::string{reqline_.path_}, std::move(up_wdgt));
    }

    // build reply object
    reply x{fd_, reqline_.path_, reqline_.query_, headers_,
            &session_->get_lut()};

    // if a new session has been created send session id at next opportunity
    if (send_session_id_in_reply_) {
      x.send_session_id_at_next_opportunity(session_id_);
      send_session_id_in_reply_ = false;
    }
    // check if request has content
    const std::string_view content_length_str = headers_["content-length"];
    if (content_length_str.empty()) {
      // no content from client, render widget
      widget_->to(x);
      // done
      state_ = next_request;
      return;
    }
    // initiate for content from client
    content_.init_for_receive(content_length_str);
    const size_t content_len = content_.content_len();
    // initiating call to widget with buf=nullptr
    widget_->on_content(x, nullptr, 0, 0, content_len);
    // if client expects 100 continue before sending post
    auto expect = headers_["expect"];
    if (expect == "100-continue") {
      io_send(fd_, "HTTP/1.1 100\r\n\r\n"sv);
      state_ = receiving_content;
      return;
    }
    // check if all the content is in the request buffer
    const size_t rem = reqbuf_.remaining();
    if (rem >= content_len) {
      // full content is in 'reqbuf'
      widget_->on_content(x, reqbuf_.ptr(), content_len, content_len,
                          content_len);
      // done
      state_ = next_request;
      return;
    }
    // beginning of the content is in 'reqbuf'
    widget_->on_content(x, reqbuf_.ptr(), rem, rem, content_len);
    // advance by the number of bytes received
    content_.unsafe_skip(rem);
    // continue receiving content
    state_ = receiving_content;
  }

  void retrieve_session_id_from_cookie() {
    // get session id from cookie or create new
    const std::string_view cookie = headers_["cookie"];
    if (cookie.starts_with("i=")) {
      // 2 to skip 'i='
      session_id_ = cookie.substr(2);
    }
  }

  void retrieve_or_create_session() {
    // check if session id is in cookie
    if (session_id_.empty()) {
      // no session id, create session id with format e.g.
      // '20150411-225519-ieu44dn'
      const time_t timer = time(nullptr);
      tm tm_info{};
      if (gmtime_r(&timer, &tm_info) == nullptr) {
        throw client_exception{"sock:retrieve_or_create_session:gmtime_r"};
      }
      std::array<char, 24> sid{};
      if (!strftime(sid.data(), sid.size(), "%Y%m%d-%H%M%S-", &tm_info)) {
        throw client_exception{"sock:do_serve_widget:1"};
      }
      // 16 is len of "20150411-225519-"
      char *sid_ptr = sid.data() + 16;
      // generate 7 random characters between 'a' and 'z'
      for (unsigned i = 0; i < 7; i++) {
        *sid_ptr++ = 'a' + char(random() % 26); // NOLINT
      }
      *sid_ptr = '\0';
      session_id_ = sid.data();
      // make unique pointer of 'session' with lifetime of 'sessions'
      auto up = std::make_unique<session>(sid.data());
      session_ = up.get();
      sessions.put(std::move(up));
      send_session_id_in_reply_ = true;
      return;
    }
    // session id in cookie. try to get from 'sessions'
    session_ = sessions.get(session_id_);
    if (session_) {
      return; // session found, done
    }
    // session not found, create
    auto up = std::make_unique<session>(std::string{session_id_});
    session_ = up.get();
    sessions.put(std::move(up));
  }

  void do_serve_upload() {
    // file upload
    std::array<char, conf::upload_path_size> pth{};
    // +1 to skip the leading '/'
    const int res = snprintf(pth.data(), pth.size(), "upload/%s",
                             reqline_.path_.substr(1).data());
    if (res < 0 or size_t(res) >= pth.size()) {
      throw client_exception{"sock:pathtrunc"};
    }
    // open file for write
    upload_fd_ =
        open(pth.data(), O_CREAT | O_WRONLY | O_TRUNC | O_CLOEXEC, 0664);
    if (upload_fd_ == -1) {
      perror("sock:do_server_upload 1");
      throw client_exception{"sock:err7"};
    }
    // handle if client expects 100-continue before sending content
    auto expect = headers_["expect"];
    if (expect == "100-continue") {
      io_send(fd_, "HTTP/1.1 100\r\n\r\n"sv);
      state_ = receiving_upload;
      return;
    }
    const size_t remaining = reqbuf_.remaining();
    if (remaining == 0) {
      state_ = receiving_upload;
      return;
    }
    // check if the whole file is in buffer by comparing
    // expected upload size with remaining unhandled
    // bytes in request buffer
    const size_t content_len = content_.content_len();
    if (remaining >= content_len) {
      // the whole file is in 'buf'
      const ssize_t n = write(upload_fd_, reqbuf_.ptr(), content_len);
      if (n == -1 or size_t(n) != content_len) {
        perror("sock:do_server_upload 2");
        throw client_exception{"sock:err4"};
      }
      // close file
      if (::close(upload_fd_)) {
        perror("sock:do_server_upload 4");
      }
      // acknowledge request complete
      io_send(fd_, "HTTP/1.1 204\r\n\r\n"sv);
      state_ = next_request;
      return;
    }
    // start of the file is in 'reqbuf' the rest will
    // be received in 'content'
    // write bytes remaining in 'reqbuf'
    const ssize_t n = write(upload_fd_, reqbuf_.ptr(), remaining);
    if (n == -1 or size_t(n) != remaining) {
      perror("sock:do_server_upload 5");
      throw client_exception{"sock:err6"};
    }
    // advance content upload position
    content_.unsafe_skip(size_t(n));
    state_ = receiving_upload;
  }

  void do_serve_file(reply &x, std::string_view path) {
    // check for illegal path containing break-out of root directory
    if (path.find("..") != std::string_view::npos) {
      x.http(403, "path contains ..\n"sv);
      state_ = next_request;
      return;
    }
    // get file info
    struct stat fdstat {};
    if (stat(path.data(), &fdstat)) {
      // error or not found
      x.http(404, "not found\n"sv);
      state_ = next_request;
      return;
    }
    // check if path is directory
    if (S_ISDIR(fdstat.st_mode)) {
      // directory not allowed
      x.http(403, "path is directory\n"sv);
      state_ = next_request;
      return;
    }
    // get modified time of file
    tm tm_info{};
    if (gmtime_r(&fdstat.st_mtime, &tm_info) == nullptr) {
      throw client_exception{"sock:do_serve_file:gmtime_r"};
    }
    // format for check with 'if-modified-since' header value
    std::array<char, 64> lastmod{};
    // e.g.: 'Fri, 31 Dec 1999 23:59:59 GMT'
    if (!strftime(lastmod.data(), lastmod.size(), "%a, %d %b %y %H:%M:%S %Z",
                  &tm_info)) {
      throw client_exception{"sock:strftime"};
    }
    // check if file has been modified since then
    const std::string_view lastmodstr = headers_["if-modified-since"];
    if (lastmodstr == lastmod.data()) {
      // not modified, reply 304
      io_send(fd_, "HTTP/1.1 304\r\n\r\n"sv);
      state_ = next_request;
      return;
    }
    // open file
    if (file_.open(path.data()) == -1) {
      // error
      x.http(404, "cannot open file\n"sv);
      state_ = next_request;
      return;
    }
    // start sending file content
    stats.files++;
    // check if ranged request
    const std::string_view range = headers_["range"];
    // format header
    std::array<char, 512> header_buf{};
    int header_buf_len = 0;
    if (!range.empty()) {
      // ranged request
      off_t offset = 0;
      if (sscanf(range.data(), "bytes=%jd", &offset) == EOF) {
        stats.errors++;
        perror("sock:do_serve_file");
        throw client_exception{"sock:do_serve_file scanf error"};
      }
      // initialize for send file starting at requested 'offset'
      file_.init_for_send(size_t(fdstat.st_size), offset);
      const size_t len = file_.length();
      // create header for ranged reply
      // todo: content-type depending on file suffix
      header_buf_len =
          snprintf(header_buf.data(), header_buf.size(),
                   "HTTP/1.1 206\r\nAccept-Ranges: "
                   "bytes\r\nLast-Modified: %s\r\nContent-Length: "
                   "%zu\r\nContent-Range: %zu-%zu/%zu\r\n\r\n",
                   lastmod.data(), len - size_t(offset), offset, len, len);
    } else {
      // not ranged request
      // initialize for send full file
      file_.init_for_send(size_t(fdstat.st_size));
      // create header for full reply
      // todo: content-type depending on file suffix
      header_buf_len =
          snprintf(header_buf.data(), header_buf.size(),
                   "HTTP/1.1 200\r\nAccept-Ranges: bytes\r\nLast-Modified: "
                   "%s\r\nContent-Length: %zu\r\n\r\n",
                   lastmod.data(), file_.length());
    }
    if (header_buf_len < 0 or size_t(header_buf_len) >= header_buf.size()) {
      throw client_exception{"sock:err1"};
    }
    // send reply with buffering of packets
    io_send(fd_, header_buf.data(), size_t(header_buf_len), true);
    // resume/start sending file
    const ssize_t n = file_.resume_send_to(fd_);
    if (n == -1) {
      if (errno == EPIPE or errno == ECONNRESET) {
        throw client_closed_exception{};
      }
      stats.errors++;
      perror("sock:do_server_file while sending");
      throw client_exception{"sock:err5"};
    }
    // check if done
    if (!file_.is_done()) {
      // not done, resume when write on socket is available
      state_ = resume_send_file;
      return;
    }
    // done, close file
    file_.close();
    state_ = next_request;
  }

  inline void io_request_read() {
    struct epoll_event ev {};
    ev.data.ptr = this;
    ev.events = EPOLLIN | EPOLLRDHUP;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd_, &ev)) {
      throw client_exception{"sock:epollmodread"};
    }
  }

  inline void io_request_write() {
    struct epoll_event ev {};
    ev.data.ptr = this;
    ev.events = EPOLLOUT | EPOLLRDHUP;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd_, &ev)) {
      throw client_exception{"sock:epollmodwrite"};
    }
  }

  class file {
    off_t offset_{};
    size_t count_{};
    int fd_{};

  public:
    inline void close() const {
      if (::close(fd_)) {
        perror("closefile");
      }
    }
    inline auto resume_send_to(const int out_fd) -> ssize_t {
      stats.writes++;
      const size_t count = count_ - size_t(offset_);
      const ssize_t n = sendfile(out_fd, fd_, &offset_, count);
      if (n == -1) { // error
        return n;
      }
      // if (size_t(n) != count) {
      //   printf("sock:file:resume_send_to sent %zd of %zu\n", n, count_);
      // }
      if (n == 0) { // file truncated
        stats.errors++;
        throw client_exception{"sock:file:resume_send_to sendfile 0"};
      }
      stats.output += size_t(n);
      return n;
    }
    inline void init_for_send(const size_t size_in_bytes,
                              const off_t seek_pos = 0) {
      offset_ = seek_pos;
      count_ = size_in_bytes;
    }
    [[nodiscard]] inline auto is_done() const -> bool {
      return size_t(offset_) == count_;
    }
    [[nodiscard]] inline auto length() const -> size_t { return count_; }
    inline auto open(const char *path) -> int {
      fd_ = ::open(path, O_RDONLY | O_CLOEXEC);
      return fd_;
    }
    inline void rst() {
      fd_ = 0;
      offset_ = 0;
      count_ = 0;
    }
  } file_{};

  struct reqline {
    std::string_view path_{};
    std::string_view query_{};
    inline void rst() {
      path_ = {};
      query_ = {};
    }
  } reqline_{};

  struct header {
    std::string_view name_{};
    std::string_view value_{};
    inline void rst() {
      name_ = {};
      value_ = {};
    }
  } header_{};

  class content {
    size_t pos_{};
    size_t len_{};
    std::unique_ptr<std::array<char, conf::sock_content_buf_size>> buf_{};

  public:
    inline void rst() { pos_ = len_ = 0; }
    [[nodiscard]] inline auto buf() const
        -> const std::array<char, conf::sock_content_buf_size> & {
      return *buf_;
    }
    [[nodiscard]] inline auto pos() const -> size_t { return pos_; }
    [[nodiscard]] inline auto remaining() const -> size_t {
      return len_ - pos_;
    }
    inline void unsafe_skip(const size_t n) { pos_ += n; }
    [[nodiscard]] inline auto content_len() const -> size_t { return len_; }

    inline void init_for_receive(const std::string_view content_length_str) {
      pos_ = 0;
      len_ = size_t(atoll(content_length_str.data()));
      // todo: abuse len
      // todo: atoll error
      if (!buf_) {
        buf_ =
            std::make_unique<std::array<char, conf::sock_content_buf_size>>();
      }
    }

    inline auto receive_from(int fd_in) -> ssize_t {
      stats.reads++;
      const ssize_t n = recv(fd_in, buf_.get(), conf::sock_content_buf_size, 0);
      if (n == -1) { // error
        return n;
      }
      if (n == 0) { // file truncated
        stats.errors++;
        throw client_exception{"sock:content:receive_from recv 0"};
      }
      // data was received
      stats.input += size_t(n);
      if (conf::print_traffic) {
        const ssize_t m = write(conf::print_traffic_fd, buf_.get(), size_t(n));
        if (m == -1 or m != n) {
          perror("reply:io_send");
        }
      }
      return n;
    }
  } content_{};

  class reqbuf {
    std::array<char, conf::sock_request_header_buf_size> buf_{};
    char *mark_{buf_.data()};
    char *p_{buf_.data()};
    char *e_{buf_.data()};

  public:
    inline void rst() { p_ = e_ = buf_.data(); }
    [[nodiscard]] inline auto ptr() const -> char * { return p_; }
    inline void set_mark() { mark_ = p_; }
    [[nodiscard]] inline auto get_mark() const -> char * { return mark_; }
    [[nodiscard]] inline auto has_more() const -> bool { return p_ != e_; }
    [[nodiscard]] inline auto remaining() const -> size_t {
      return size_t(e_ - p_);
    }
    inline void unsafe_skip(const size_t n) { p_ += n; }
    inline auto unsafe_next_char() -> char { return *p_++; }
    inline void set_eos() { *(p_ - 1) = '\0'; }
    inline auto string_view_from_mark() -> std::string_view {
      char *m = mark_;
      mark_ = p_;
      // -1 because 'p_' is 1 character past '\0'
      return {m, size_t(p_ - m - 1)};
    }
    inline auto receive_from(const int fd_in) -> ssize_t {
      const size_t nbytes_to_read =
          conf::sock_request_header_buf_size - size_t(p_ - buf_.data());
      if (nbytes_to_read == 0) {
        throw client_exception{"sock:buf:full"};
      }
      stats.reads++;
      const ssize_t n = recv(fd_in, p_, nbytes_to_read, 0);
      if (n == -1) {
        return n;
      }
      // when "Too many open files" recv returns 0 making a busy loop
      if (n == 0) {
        stats.errors++;
        throw client_exception{"sock:buf:receive_from recv 0"};
      }
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
  } reqbuf_{};

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
  } state_{method};

  int fd_{};
  struct sockaddr_in sock_addr_ {};
  map_headers headers_{};
  int upload_fd_{};
  std::string session_id_{};
  // note
  // 'widget' lifetime is same as 'session'
  // 'session' lifetime is same as 'sessions'
  // 'sessions' lifetime is same as program > 'sock' lifetime
  // raw pointers ok
  widget *widget_{};
  session *session_{};
  bool send_session_id_in_reply_{};

  inline static auto trim(std::string_view in) -> std::string_view {
    const auto *left = in.begin();
    while (true) {
      if (left == in.end()) {
        return {};
      }
      if (!isspace(*left)) {
        break;
      }
      ++left;
    }
    const auto *right = in.end() - 1;
    while (right > left && isspace(*right)) {
      --right;
    }
    return {left, size_t(std::distance(left, right) + 1)};
  }

  inline static void strlwr(char *p) {
    while (*p) {
      *p = char(tolower(*p));
      p++;
    }
  }
};
} // namespace xiinux
