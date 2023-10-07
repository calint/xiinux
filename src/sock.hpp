// reviewed: 2023-09-27
//           2023-10-03
#pragma once
#include "conf.hpp"
#include "decouple.hpp"
#include "doc.hpp"
#include "sessions.hpp"
#include "web/web.hpp"
#include <fcntl.h>
#include <filesystem>
#include <memory>
#include <netinet/in.h>
#include <string_view>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>

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
      printf("%s  %s  session=[%s]  disconnect fd=%d\n",
             current_time_to_str(time_str_buf),
             ip_addr_to_str(ip_str_buf, &(sock_addr_.sin_addr.s_addr)),
             session_id_.empty() ? "n/a" : session_id_.data(), fd_);
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
        if (close(upload_fd_)) {
          perror("sock:closing upload file");
        }
        // set last modified
        const struct utimbuf tm {
          upload_last_mod_, upload_last_mod_
        };
        if (utime(upload_path_.c_str(), &tm)) {
          throw client_exception(
              "sock:run:receiving_content could not set file modified time");
        }
        send_http_response(204);
        state_ = next_request;
      }
      if (state_ == next_request) {
        stats.requests++;
        reqbuf_.rst();
        reqline_.rst();
        header_.rst();
        headers_.clear();
        content_.rst();
        file_.rst();
        upload_fd_ = 0;
        upload_last_mod_ = 0;
        upload_path_.clear();
        widget_ = nullptr;
        session_id_ = {};
        send_session_id_in_reply_ = false;
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
            char *bgn = reqbuf_.get_mark();
            const char *end = urldecode(bgn);
            reqline_.path_ = {bgn, size_t(end - bgn)};
            state_ = protocol;
            break;
          }
          if (ch == '?') {
            reqbuf_.set_eos();
            char *bgn = reqbuf_.get_mark();
            const char *end = urldecode(bgn);
            reqline_.path_ = {bgn, size_t(end - bgn)};
            reqbuf_.set_mark();
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
  inline auto get_path() const -> const std::string_view & {
    return reqline_.path_;
  }
  inline auto get_query() const -> const std::string_view & {
    return reqline_.query_;
  }
  // inline auto get_headers() const -> const map_headers & { return headers_; }
  // inline auto get_session() const -> session * { return session_; }
  inline auto get_session_id() const -> const std::string & {
    return session_id_;
  }

private:
  inline void do_after_headers() {

    retrieve_session_id_from_cookie();

    if constexpr (conf::sock_print_client_requests) {
      // client ip
      std::array<char, INET_ADDRSTRLEN> ip_addr_str{};
      in_addr_t addr = get_socket_address().sin_addr.s_addr;

      // current time
      std::array<char, 26> time_str_buf{};
      current_time_to_str(time_str_buf);

      // output
      printf("%s  %s  session=[%s]  path=[%s]  query=[%s]\n",
             time_str_buf.data(), ip_addr_to_str(ip_addr_str, &addr),
             session_id_.empty() ? "n/a" : session_id_.c_str(),
             reqline_.path_.data(), reqline_.query_.data());
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
    const std::string_view &content_type = headers_["content-type"];
    if (content_type.starts_with("file;")) {
      // extract millis since January 1, 1970 (Unix timestamp)
      const size_t ix = content_type.find(';');
      const std::string_view &last_mod = content_type.substr(ix + 1);
      // convert to seconds
      upload_last_mod_ = time_t(std::stoul(std::string{last_mod}) / 1000);
      // this is an upload, initiate content receive
      content_.init_for_receive(headers_["content-length"]);
      retrieve_or_create_session();
      do_serve_upload();
      return;
    }

    // create reply
    reply x{fd_, reqline_.path_, reqline_.query_, headers_, nullptr};

    // remove leading '/' from path if exists
    const std::string_view &path =
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
    const std::string_view &content_length_str = headers_["content-length"];
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
      send_http_response(100);
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

  inline void retrieve_session_id_from_cookie() {
    // get session id from cookie or create new
    const std::string_view &cookie = headers_["cookie"];
    if (cookie.starts_with("i=")) {
      // 2 to skip 'i='
      session_id_ = cookie.substr(2);
    }
  }

  inline void retrieve_or_create_session() {
    // check if session id is in cookie
    if (session_id_.empty()) {
      // no session id, create session id with format e.g.
      // '20150411-225519-ieu44dna'
      const time_t timer = time(nullptr);
      tm tm_info{};
      if (gmtime_r(&timer, &tm_info) == nullptr) {
        throw client_exception{"sock:retrieve_or_create_session:gmtime_r"};
      }
      std::array<char, 24> sid{};
      const size_t sid_len =
          strftime(sid.data(), sid.size(), "%Y%m%d-%H%M%S-", &tm_info);
      if (sid_len == 0) {
        throw client_exception{"sock:do_serve_widget:1"};
      }
      // e.g. sid="20231006-145203-"
      // remaining part of 'sid' is random characters
      for (size_t i = sid_len; i < sid.size(); i++) {
        sid[i] = 'a' + char(random() % 26); // NOLINT
      }
      session_id_ = {sid.data(), sid.size()};
      // make unique pointer of 'session' with lifetime of 'sessions'
      auto up_ses = std::make_unique<session>(session_id_);
      // lifetime of raw pointer ok. see member declaration comment
      session_ = up_ses.get();
      sessions.put(std::move(up_ses));
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

  inline void do_serve_upload() {
    // file upload
    strb<conf::upload_path_size> sb{};
    sb.p("u/"sv).p(session_id_).p('/').p(reqline_.path_.substr(1)).eos();

    // create directory if it does not exist
    namespace fs = std::filesystem;
    const fs::path fs_pth = fs::path(sb.string_view());
    const fs::path fs_pth_dir = fs_pth.parent_path();
    if (!fs::exists(fs_pth_dir)) {
      if (!fs::create_directories(fs_pth_dir)) {
        throw client_exception("sock:do_server_upload:1");
      }
    }
    //  else { //? this check might be un-necessary since 'open' will fail
    //   if (!fs::is_directory(fs_pth_dir)) {
    //     throw client_exception("sock:do_server_upload:2");
    //   }
    // }

    upload_path_ = fs_pth.string();

    // open file for write
    upload_fd_ = open(upload_path_.data(),
                      O_CREAT | O_WRONLY | O_TRUNC | O_CLOEXEC, 0664);
    if (upload_fd_ == -1) {
      perror("sock:do_serve_upload 1");
      throw client_exception{"sock:err7"};
    }
    // handle if client expects 100-continue before sending content
    const std::string_view &expect = headers_["expect"];
    if (expect == "100-continue") {
      send_http_response(100);
      state_ = receiving_upload;
      return;
    }
    // check if the whole file is in buffer by comparing
    // expected upload size with remaining unhandled
    // bytes in request buffer
    const size_t remaining = reqbuf_.remaining();
    const size_t content_len = content_.content_len();
    if (remaining >= content_len) {
      // the whole file is in 'reqbuf'
      const ssize_t n = write(upload_fd_, reqbuf_.ptr(), content_len);
      if (n == -1 or size_t(n) != content_len) {
        perror("sock:do_server_upload 2");
        throw client_exception{"sock:err4"};
      }
      // close file
      if (close(upload_fd_)) {
        perror("sock:do_server_upload 3");
        throw client_exception("sock:do_server_upload: 3");
      }
      // set last modified
      const struct utimbuf tm {
        upload_last_mod_, upload_last_mod_
      };
      if (utime(upload_path_.data(), &tm)) {
        throw client_exception("sock:do_server_upload: 4");
      }
      // acknowledge request complete
      send_http_response(204);
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

  inline void do_serve_file(reply &x, const std::string_view &path) {
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
    // will back the string_view of 'path_resolved' incase it is the default
    // directory file
    // note. ? placed inside the 'if' would be U.B.?
    //       not caught by compiler, clang-tidy or valgrind
    strb<path_max_size> path_buf{};
    std::string_view path_resolved{};
    // check if path is directory
    if (S_ISDIR(fdstat.st_mode)) {
      // check for default directory file
      path_buf.p(path).p('/').p(directory_default_file).eos();
      if (stat(path_buf.buf(), &fdstat)) {
        // error or not found
        x.http(404, "not found\n"sv);
        state_ = next_request;
        return;
      }
      path_resolved = path_buf.string_view();
    } else {
      path_resolved = path;
    }
    // get modified time of file
    tm tm_info{};
    if (gmtime_r(&fdstat.st_mtime, &tm_info) == nullptr) {
      throw client_exception{"sock:do_serve_file:gmtime_r"};
    }
    // format for check with 'if-modified-since' header value
    std::array<char, 64> lastmod{};
    // e.g.: 'Fri, 31 Dec 1999 23:59:59 GMT'
    const size_t lastmod_len = strftime(lastmod.data(), lastmod.size(),
                                        "%a, %d %b %y %H:%M:%S %Z", &tm_info);
    if (lastmod_len == 0) {
      throw client_exception{"sock:strftime"};
    }
    // check if file has been modified since then
    const std::string_view &lastmodstr = headers_["if-modified-since"];
    if (lastmodstr == lastmod.data()) {
      // not modified, reply 304
      send_http_response(304);
      state_ = next_request;
      return;
    }
    // open file
    if (file_.open(path_resolved) == -1) {
      // error
      x.http(404, "cannot open file\n"sv);
      state_ = next_request;
      return;
    }
    // start sending file content
    stats.files++;
    // check if ranged request
    const std::string_view &range = headers_["range"];
    // format header
    strb<conf::sock_response_header_buffer_size> sb{};
    sb.p("HTTP/1.1 "sv)
        .p(range.empty() ? "200"sv : "206"sv)
        .p("\r\nAccept-Ranges: bytes\r\nLast-Modified: "sv)
        .p({lastmod.data(), lastmod_len});

    // content-type
    const size_t suffix_ix = path_resolved.find_last_of('.');
    if (suffix_ix != std::string_view::npos) {
      const std::string_view &suffix = path_resolved.substr(suffix_ix + 1);
      if (suffix == "js") {
        // fixes warning in firefox for javascript files
        sb.p("\r\nContent-Type: application/javascript"sv);
      }
    }

    sb.p("\r\nContent-Length: "sv);

    const auto file_len = size_t(fdstat.st_size);

    if (!range.empty()) {
      // ranged request
      off_t offset = 0;
      if (sscanf(range.data(), "bytes=%jd", &offset) == EOF) {
        stats.errors++;
        perror("sock:do_serve_file");
        throw client_exception{"sock:do_serve_file scanf error"};
      }
      // create header for ranged reply
      // todo: content-type depending on file suffix
      sb.p(file_len - size_t(offset))
          .p("\r\nContent-Range: "sv)
          .p(size_t(offset))
          .p('-')
          .p(file_len)
          .p('/')
          .p(file_len);
      // initialize for send file starting at requested 'offset'
      file_.init_for_send(file_len, offset);
    } else {
      // not ranged request
      // create header for full reply
      // todo: content-type depending on file suffix
      sb.p(file_len);
      // initialize for send full file
      file_.init_for_send(file_len, 0);
    }
    sb.p("\r\n\r\n"sv); // note. no eos() because it would be included in the
                        // send
    // send reply with buffering of packets
    io_send(fd_, sb.string_view(), true);
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

  inline void send_http_response(const int http_code) {
    strb<conf::sock_response_header_buffer_size> sb{};
    sb.p("HTTP/1.1 "sv).p(http_code).p("\r\n"sv);
    if (send_session_id_in_reply_) {
      sb.p("Set-Cookie: i="sv)
          .p(session_id_)
          .p(";path=/;expires=Thu, 31-Dec-2099 00:00:00 GMT;SameSite=Lax\r\n\r\n"sv);
      send_session_id_in_reply_ = false;
    }
    sb.p("\r\n"sv).eos();
    io_send(fd_, sb.string_view());
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
      if (n == 0 and count != 0) { // file truncated
        stats.errors++;
        throw client_exception{"sock:file:resume_send_to sendfile 0"};
      }
      stats.output += size_t(n);
      return n;
    }
    inline void init_for_send(const size_t size_in_bytes,
                              const off_t seek_pos) {
      offset_ = seek_pos;
      count_ = size_in_bytes;
    }
    [[nodiscard]] inline auto is_done() const -> bool {
      return size_t(offset_) == count_;
    }
    // [[nodiscard]] inline auto length() const -> size_t { return count_; }
    inline auto open(const std::string_view &path) -> int {
      fd_ = ::open(path.data(), O_RDONLY | O_CLOEXEC);
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

    inline void init_for_receive(const std::string_view &content_length_str) {
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
      if constexpr (conf::print_traffic) {
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
    // inline void unsafe_skip(const size_t n) { p_ += n; }
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
      if constexpr (conf::print_traffic) {
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
  time_t upload_last_mod_{};
  std::string upload_path_{};
  std::string session_id_{};
  // note
  // 'widget' lifetime is same as 'session'
  // 'session' lifetime is same as 'sessions'
  // 'sessions' lifetime is same as program > 'sock' lifetime
  // raw pointers ok
  widget *widget_{};
  session *session_{};
  bool send_session_id_in_reply_{};

  inline static auto trim(const std::string_view &in) -> std::string_view {
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

  inline static auto urldecode(char *str) -> char * {
    const char *p = str;
    while (*p) {
      if (*p == '+') {
        *str++ = ' ';
        p++;
        continue;
      }
      char a = p[1]; // +1 might be '\0'
      char b = a == '\0' ? '\0' : p[2];
      if (*p == '%' and a and b and isxdigit(a) and isxdigit(b)) {
        if (a >= 'a') {
          a -= 'a' - 'A';
        }
        if (a >= 'A') {
          a -= 'A' - 10;
        } else {
          a -= '0';
        }
        if (b >= 'a') {
          b -= 'a' - 'A';
        }
        if (b >= 'A') {
          b -= 'A' - 10;
        } else {
          b -= '0';
        }
        *str++ = char(16 * a + b);
        p += 3;
        continue;
      }
      *str++ = *p++;
    }
    *str = '\0';
    return str;
  }

  static constexpr size_t path_max_size = 256;
  static constexpr const char *directory_default_file = "index.html";
};
} // namespace xiinux
