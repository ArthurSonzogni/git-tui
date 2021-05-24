/**
 * @file process.h
 * @author Chase Geigle
 *
 * A simple, header-only process/pipe library for C++ on UNIX platforms.
 *
 * Released under the MIT license (see LICENSE).
 */

#ifndef PROCXX_PROCESS_H_
#define PROCXX_PROCESS_H_

#include <fcntl.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <istream>
#include <mutex>
#include <ostream>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <system_error>
#include <vector>

#ifndef PROCXX_HAS_PIPE2
#define PROCXX_HAS_PIPE2 1
#endif

namespace procxx {

/**
 * Represents a UNIX pipe between processes.
 */
class pipe_t {
 public:
  static constexpr unsigned int READ_END = 0;
  static constexpr unsigned int WRITE_END = 1;

  /**
   * Wrapper type that ensures sanity when dealing with operations on
   * the different ends of the pipe.
   */
  class pipe_end {
   public:
    /**
     * Constructs a new object to represent an end of a pipe. Ensures
     * the end passed makes sense (e.g., is either the READ_END or the
     * WRITE_END of the pipe).
     */
    pipe_end(unsigned int end) {
      if (end != READ_END && end != WRITE_END)
        throw exception{"invalid pipe end"};
      end_ = end;
    }

    /**
     * pipe_ends are implicitly convertible to ints.
     */
    operator unsigned int() const { return end_; }

   private:
    unsigned int end_;
  };

  /**
   * Gets a pipe_end representing the read end of a pipe.
   */
  static pipe_end read_end() {
    static pipe_end read{READ_END};
    return read;
  }

  /**
   * Gets a pipe_end representing the write end of a pipe.
   */
  static pipe_end write_end() {
    static pipe_end write{WRITE_END};
    return write;
  }

  /**
   * Constructs a new pipe.
   */
  pipe_t() {
#if PROCXX_HAS_PIPE2
    const auto r = ::pipe2(&pipe_[0], O_CLOEXEC);
    if (-1 == r)
      throw exception("pipe2 failed: " + std::system_category().message(errno));
#else
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock{mutex};
    ::pipe(&pipe_[0]);

    auto flags = ::fcntl(pipe_[0], F_GETFD, 0);
    ::fcntl(pipe_[0], F_SETFD, flags | FD_CLOEXEC);

    flags = ::fcntl(pipe_[1], F_GETFD, 0);
    ::fcntl(pipe_[1], F_SETFD, flags | FD_CLOEXEC);
#endif
  }

  /**
   * Pipes may be move constructed.
   */
  pipe_t(pipe_t&& other) {
    pipe_ = std::move(other.pipe_);
    other.pipe_[READ_END] = -1;
    other.pipe_[WRITE_END] = -1;
  }

  /**
   * Pipes are unique---they cannot be copied.
   */
  pipe_t(const pipe_t&) = delete;

  /**
   * Writes length bytes from buf to the pipe.
   *
   * @param buf the buffer to get bytes from
   * @param length the number of bytes to write
   */
  void write(const char* buf, uint64_t length) {
    auto bytes = ::write(pipe_[WRITE_END], buf, length);
    if (bytes == -1) {
      // interrupt, just attempt to write again
      if (errno == EINTR)
        return write(buf, length);
      // otherwise, unrecoverable error
      perror("pipe_t::write()");
      throw exception{"failed to write"};
    }
    if (bytes < static_cast<ssize_t>(length))
      write(buf + bytes, length - static_cast<uint64_t>(bytes));
  }

  /**
   * Reads up to length bytes from the pipe, placing them in buf.
   *
   * @param buf the buffer to write to
   * @param length the maximum number of bytes to read
   * @return the actual number of bytes read
   */
  ssize_t read(char* buf, uint64_t length) {
    auto bytes = ::read(pipe_[READ_END], buf, length);
    return bytes;
  }

  /**
   * Closes both ends of the pipe.
   */
  void close() {
    close(read_end());
    close(write_end());
  }

  /**
   * Closes a specific end of the pipe.
   */
  void close(pipe_end end) {
    if (pipe_[end] != -1) {
      ::close(pipe_[end]);
      pipe_[end] = -1;
    }
  }

  /**
   * Determines if an end of the pipe is still open.
   */
  bool open(pipe_end end) { return pipe_[end] != -1; }

  /**
   * Redirects the given file descriptor to the given end of the pipe.
   *
   * @param end the end of the pipe to connect to the file descriptor
   * @param fd the file descriptor to connect
   */
  void dup(pipe_end end, int fd) {
    if (::dup2(pipe_[end], fd) == -1) {
      perror("pipe_t::dup()");
      throw exception{"failed to dup"};
    }
  }

  /**
   * Redirects the given end of the given pipe to the current pipe.
   *
   * @param end the end of the pipe to redirect
   * @param other the pipe to redirect to the current pipe
   */
  void dup(pipe_end end, pipe_t& other) { dup(end, other.pipe_[end]); }

  /**
   * The destructor for pipes relinquishes any file descriptors that
   * have not yet been closed.
   */
  ~pipe_t() { close(); }

  /**
   * An exception type for any unrecoverable errors that occur during
   * pipe operations.
   */
  class exception : public std::runtime_error {
   public:
    using std::runtime_error::runtime_error;
  };

 private:
  std::array<int, 2> pipe_;
};

/**
 * Streambuf for reading/writing to pipes.
 *
 * @see http://www.mr-edd.co.uk/blog/beginners_guide_streambuf
 */
class pipe_ostreambuf : public std::streambuf {
 public:
  /**
   * Constructs a new streambuf, with the given buffer size and put_back
   * buffer space.
   */
  pipe_ostreambuf(size_t buffer_size = 512, size_t put_back_size = 8)
      : put_back_size_{put_back_size}, in_buffer_(buffer_size + put_back_size) {
    auto end = &in_buffer_.back() + 1;
    setg(end, end, end);
  }

  ~pipe_ostreambuf() = default;

  int_type underflow() override {
    // if the buffer is not exhausted, return the next element
    if (gptr() < egptr())
      return traits_type::to_int_type(*gptr());

    auto base = &in_buffer_.front();
    auto start = base;

    // if we are not the first fill of the buffer
    if (eback() == base) {
      // move the put_back area to the front
      const auto dest = base;
      const auto src =
          egptr() - put_back_size_ < dest ? dest : egptr() - put_back_size_;
      const auto area =
          static_cast<std::size_t>(egptr() - dest) < put_back_size_
              ? static_cast<std::size_t>(egptr() - dest)
              : put_back_size_;
      std::memmove(dest, src, area);
      start += put_back_size_;
    }

    // start now points to the head of the usable area of the buffer
    auto bytes = stdout_pipe_.read(
        start, in_buffer_.size() - static_cast<std::size_t>(start - base));

    if (bytes == -1) {
      ::perror("read");
      throw exception{"failed to read from pipe"};
    }

    if (bytes == 0)
      return traits_type::eof();

    setg(base, start, start + bytes);

    return traits_type::to_int_type(*gptr());
  }

  /**
   * An exception for pipe_streambuf interactions.
   */
  class exception : public std::runtime_error {
   public:
    using std::runtime_error::runtime_error;
  };

  /**
   * Gets the stdout pipe.
   */
  pipe_t& stdout_pipe() { return stdout_pipe_; }

  /**
   * Closes one of the pipes. This will flush any remaining bytes in the
   * output buffer.
   */
  virtual void close(pipe_t::pipe_end end) {
    if (end == pipe_t::read_end())
      stdout_pipe().close(pipe_t::read_end());
  }

 protected:
  virtual void flush() {}

  size_t put_back_size_;
  pipe_t stdout_pipe_;
  std::vector<char> in_buffer_;
};

class pipe_streambuf : public pipe_ostreambuf {
 public:
  pipe_streambuf(size_t buffer_size = 512, size_t put_back_size = 8)
      : pipe_ostreambuf{buffer_size, put_back_size},
        out_buffer_(buffer_size + 1) {
    auto begin = &out_buffer_.front();
    setp(begin, begin + out_buffer_.size() - 1);
  }

  /**
   * Destroys the streambuf, which will flush any remaining content on
   * the output buffer.
   */
  ~pipe_streambuf() { flush(); }

  int_type overflow(int_type ch) override {
    if (ch != traits_type::eof()) {
      *pptr() = static_cast<char>(ch);  // safe because of -1 in setp() in ctor
      pbump(1);
      flush();
      return ch;
    }

    return traits_type::eof();
  }

  int sync() override {
    flush();
    return 0;
  }

  /**
   * Gets the stdin pipe.
   */
  pipe_t& stdin_pipe() { return stdin_pipe_; }

  void close(pipe_t::pipe_end end) override {
    pipe_ostreambuf::close(end);
    if (end != pipe_t::read_end()) {
      flush();
      stdin_pipe().close(pipe_t::write_end());
    }
  }

 private:
  void flush() override {
    if (stdin_pipe_.open(pipe_t::write_end())) {
      stdin_pipe_.write(pbase(), static_cast<std::size_t>(pptr() - pbase()));
      pbump(static_cast<int>(-(pptr() - pbase())));
    }
  }

  pipe_t stdin_pipe_;
  std::vector<char> out_buffer_;
};

class process;

// Forward declaration. Will be defined later.
bool running(pid_t pid);
bool running(const process& pr);

/**
 * A handle that represents a child process.
 */
class process {
 public:
  /**
   * Constructs a new child process, executing the given application and
   * passing the given arguments to it.
   */
  template <class... Args>
  process(std::string application, Args&&... args)
      : args_{std::move(application), std::forward<Args>(args)...},
        in_stream_{&pipe_buf_},
        out_stream_{&pipe_buf_},
        err_stream_{&err_buf_} {
    // nothing
  }

  /*
   * Adds an argument to the argument-list
   */
  void add_argument(std::string arg) { args_.push_back(std::move(arg)); }

  /*
   * Add further arguments to the argument-list
   */
  template <typename InputIterator>
  void append_arguments(InputIterator first, InputIterator last) {
    args_.emplace(args_.end(), first, last);
  }

  /**
   * Sets the process to read from the standard output of another
   * process.
   */
  void read_from(process& other) { read_from_ = &other; }

  /**
   * Executes the process.
   */
  void exec() {
    if (pid_ != -1)
      throw exception{"process already started"};

    pipe_t err_pipe;

    auto pid = fork();
    if (pid == -1) {
      perror("fork()");
      throw exception{"Failed to fork child process"};
    } else if (pid == 0) {
      err_pipe.close(pipe_t::read_end());
      pipe_buf_.stdin_pipe().close(pipe_t::write_end());
      pipe_buf_.stdout_pipe().close(pipe_t::read_end());
      pipe_buf_.stdout_pipe().dup(pipe_t::write_end(), STDOUT_FILENO);
      err_buf_.stdout_pipe().close(pipe_t::read_end());
      err_buf_.stdout_pipe().dup(pipe_t::write_end(), STDERR_FILENO);

      if (read_from_) {
        read_from_->recursive_close_stdin();
        pipe_buf_.stdin_pipe().close(pipe_t::read_end());
        read_from_->pipe_buf_.stdout_pipe().dup(pipe_t::read_end(),
                                                STDIN_FILENO);
      } else {
        pipe_buf_.stdin_pipe().dup(pipe_t::read_end(), STDIN_FILENO);
      }

      std::vector<char*> args;
      args.reserve(args_.size() + 1);
      for (auto& arg : args_)
        args.push_back(const_cast<char*>(arg.c_str()));
      args.push_back(nullptr);

      limits_.set_limits();
      execvp(args[0], args.data());

      char err[sizeof(int)];
      std::memcpy(err, &errno, sizeof(int));
      err_pipe.write(err, sizeof(int));
      err_pipe.close();
      std::_Exit(EXIT_FAILURE);
    } else {
      err_pipe.close(pipe_t::write_end());
      pipe_buf_.stdout_pipe().close(pipe_t::write_end());
      err_buf_.stdout_pipe().close(pipe_t::write_end());
      pipe_buf_.stdin_pipe().close(pipe_t::read_end());
      if (read_from_) {
        pipe_buf_.stdin_pipe().close(pipe_t::write_end());
        read_from_->pipe_buf_.stdout_pipe().close(pipe_t::read_end());
        read_from_->err_buf_.stdout_pipe().close(pipe_t::read_end());
      }
      pid_ = pid;

      char err[sizeof(int)];
      auto bytes = err_pipe.read(err, sizeof(int));
      if (bytes == sizeof(int)) {
        int ec = 0;
        std::memcpy(&ec, err, sizeof(int));
        throw exception{"Failed to exec process: " +
                        std::system_category().message(ec)};
      } else {
        err_pipe.close();
      }
    }
  }

  /**
   * Process handles are unique: they may not be copied.
   */
  process(const process&) = delete;

  /**
   * The destructor for a process will wait for the child if client code
   * has not already explicitly waited for it.
   */
  ~process() { wait(); }

  /**
   * Gets the process id.
   */
  pid_t id() const { return pid_; }

  /**
   * Simple wrapper for process limit settings. Currently supports
   * setting processing time and memory usage limits.
   */
  class limits_t {
   public:
    /**
     * Sets the maximum amount of cpu time, in seconds.
     */
    void cpu_time(rlim_t max) {
      lim_cpu_ = true;
      cpu_.rlim_cur = cpu_.rlim_max = max;
    }

    /**
     * Sets the maximum allowed memory usage, in bytes.
     */
    void memory(rlim_t max) {
      lim_as_ = true;
      as_.rlim_cur = as_.rlim_max = max;
    }

    /**
     * Applies the set limits to the current process.
     */
    void set_limits() {
      if (lim_cpu_ && setrlimit(RLIMIT_CPU, &cpu_) != 0) {
        perror("limits_t::set_limits()");
        throw exception{"Failed to set cpu time limit"};
      }

      if (lim_as_ && setrlimit(RLIMIT_AS, &as_) != 0) {
        perror("limits_t::set_limits()");
        throw exception{"Failed to set memory limit"};
      }
    }

   private:
    bool lim_cpu_ = false;
    rlimit cpu_;
    bool lim_as_ = false;
    rlimit as_;
  };

  /**
   * Sets the limits for this process.
   */
  void limit(const limits_t& limits) { limits_ = limits; }

  /**
   * Waits for the child to exit.
   */
  void wait() {
    if (!waited_) {
      pipe_buf_.close(pipe_t::write_end());
      err_buf_.close(pipe_t::write_end());
      waitpid(pid_, &status_, 0);
      pid_ = -1;
      waited_ = true;
    }
  }

  /**
   * It wait() already called?
   */
  bool waited() const { return waited_; }

  /**
   * Determines if process is running.
   */
  bool running() const { return ::procxx::running(*this); }

  /**
   * Determines if the child exited properly.
   */
  bool exited() const {
    if (!waited_)
      throw exception{"process::wait() not yet called"};
    return WIFEXITED(status_);
  }

  /**
   * Determines if the child was killed.
   */
  bool killed() const {
    if (!waited_)
      throw exception{"process::wait() not yet called"};
    return WIFSIGNALED(status_);
  }

  /**
   * Determines if the child was stopped.
   */
  bool stopped() const {
    if (!waited_)
      throw exception{"process::wait() not yet called"};
    return WIFSTOPPED(status_);
  }

  /**
   * Gets the exit code for the child. If it was killed or stopped, the
   * signal that did so is returned instead.
   */
  int code() const {
    if (!waited_)
      throw exception{"process::wait() not yet called"};
    if (exited())
      return WEXITSTATUS(status_);
    if (killed())
      return WTERMSIG(status_);
    if (stopped())
      return WSTOPSIG(status_);
    return -1;
  }

  /**
   * Closes the given end of the pipe.
   */
  void close(pipe_t::pipe_end end) {
    pipe_buf_.close(end);
    err_buf_.close(end);
  }

  /**
   * Write operator.
   */
  template <class T>
  friend std::ostream& operator<<(process& proc, T&& input) {
    return proc.in_stream_ << input;
  }

  /**
   * Conversion to std::ostream.
   */
  std::ostream& input() { return in_stream_; }

  /**
   * Conversion to std::istream.
   */
  std::istream& output() { return out_stream_; }

  /**
   * Conversion to std::istream.
   */
  std::istream& error() { return err_stream_; }

  /**
   * Read operator.
   */
  template <class T>
  friend std::istream& operator>>(process& proc, T& output) {
    return proc.out_stream_ >> output;
  }

  /**
   * An exception type for any unrecoverable errors that occur during
   * process operations.
   */
  class exception : public std::runtime_error {
   public:
    using std::runtime_error::runtime_error;
  };

 private:
  void recursive_close_stdin() {
    pipe_buf_.stdin_pipe().close();
    if (read_from_)
      read_from_->recursive_close_stdin();
  }

  std::vector<std::string> args_;
  process* read_from_ = nullptr;
  limits_t limits_;
  pid_t pid_ = -1;
  pipe_streambuf pipe_buf_;
  pipe_ostreambuf err_buf_;
  std::ostream in_stream_;
  std::istream out_stream_;
  std::istream err_stream_;
  bool waited_ = false;
  int status_;
};

/**
 * Class that represents a pipeline of child processes. The process objects
 * that are part of the pipeline are assumed to live longer than or as long
 * as the pipeline itself---the pipeline does not take ownership of the
 * processes.
 */
class pipeline {
 public:
  friend pipeline operator|(process& first, process& second);

  /**
   * Constructs a longer pipeline by adding an additional process.
   */
  pipeline& operator|(process& tail) {
    tail.read_from(processes_.back());
    processes_.emplace_back(tail);
    return *this;
  }

  /**
   * Sets limits on all processes in the pipieline.
   */
  pipeline& limit(process::limits_t limits) {
    for_each([limits](process& p) { p.limit(limits); });
    return *this;
  }

  /**
   * Executes all processes in the pipeline.
   */
  void exec() const {
    for_each([](process& proc) { proc.exec(); });
  }

  /**
   * Obtains the process at the head of the pipeline.
   */
  process& head() const { return processes_.front(); }

  /**
   * Obtains the process at the tail of the pipeline.
   */
  process& tail() const { return processes_.back(); }

  /**
   * Waits for all processes in the pipeline to finish.
   */
  void wait() const {
    for_each([](process& proc) { proc.wait(); });
  }

  /**
   * Performs an operation on each process in the pipeline.
   */
  template <class Function>
  void for_each(Function&& function) const {
    for (auto& proc : processes_)
      function(proc.get());
  }

 private:
  explicit pipeline(process& head) : processes_{std::ref(head)} {
    // nothing
  }

  std::vector<std::reference_wrapper<process>> processes_;
};

/**
 * Begins constructing a pipeline from two processes.
 */
inline pipeline operator|(process& first, process& second) {
  pipeline p{first};
  return p | second;
}

/**
 * Determines if process is running (zombies are seen as running).
 */
inline bool running(pid_t pid) {
  bool result = false;
  if (pid != -1) {
    if (0 == ::kill(pid, 0)) {
      int status;
      const auto r = ::waitpid(pid, &status, WNOHANG);
      if (-1 == r) {
        perror("waitpid()");
        throw process::exception{
            "Failed to check process state "
            "by waitpid(): " +
            std::system_category().message(errno)};
      }
      if (r == pid)
        // Process has changed its state. We must detect why.
        result = !WIFEXITED(status) && !WIFSIGNALED(status);
      else
        // No changes in the process status. It means that
        // process is running.
        result = true;
    }
  }

  return result;
}

/**
 * Determines if process is running (zombies are seen as running).
 */
inline bool running(const process& pr) {
  return running(pr.id());
}

}  // namespace procxx

#endif

// Copyright 2021 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
