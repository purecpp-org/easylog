#pragma once

#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <functional>
#include <type_traits>

#include "util/time_util.hpp"
#ifdef __linux__
#include <sys/syscall.h>
#include <unistd.h>
#endif
#if __has_include(<memory_resource>)
#include <memory_resource>
#endif
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#ifndef FMT_HEADER_ONLY
#define FMT_HEADER_ONLY
#endif
#include "fmt/format.h"
#include "fmt/printf.h"
#include "util/dragonbox_to_chars.h"
#include "util/meta_string.hpp"
#include "util/type_traits.h"

#if defined(_WIN32)
#ifndef _WINDOWS_
#ifndef WIN32_LEAN_AND_MEAN // Sorry for the inconvenience. Please include any
                            // related headers you need manually.
                            // (https://stackoverflow.com/a/8294669)
#define WIN32_LEAN_AND_MEAN // Prevent inclusion of WinSock2.h
#endif
#include <Windows.h> // Force inclusion of WinGDI here to resolve name conflict
#endif
#ifdef ERROR // Should be true unless someone else undef'd it already
#undef ERROR // Windows GDI defines this macro; make it a global enum so it
             // doesn't conflict with our code
enum { ERROR = 0 };
#endif
#endif

namespace easylog {

namespace detail {
template <class T>
constexpr inline bool c_array_v =
    std::is_array_v<remove_cvref_t<T>> &&std::extent_v<remove_cvref_t<T>> > 0;

template <typename Type, typename = void> struct has_data : std::false_type {};

template <typename T>
struct has_data<T, std::void_t<decltype(std::declval<T>().data())>>
    : std::true_type {};

template <typename T>
constexpr inline bool has_data_v = has_data<remove_cvref_t<T>>::value;

template <typename Type, typename = void> struct has_str : std::false_type {};

template <typename T>
struct has_str<T, std::void_t<decltype(std::declval<T>().str())>>
    : std::true_type {};

template <typename T>
constexpr inline bool has_str_v = has_str<remove_cvref_t<T>>::value;
} // namespace detail

enum class FormatType {
  SPRINTF,
  FORMAT,
};

enum class Severity {
  NONE,
  TRACE,
  DEBUG,
  INFO,
  WARN,
  WARNING = WARN,
  ERROR,
  CRITICAL,
  FATAL = CRITICAL,
};

inline std::string_view severity_str(Severity severity) {
  switch (severity) {
  case Severity::TRACE:
    return "TRACE   ";
  case Severity::DEBUG:
    return "DEBUG   ";
  case Severity::INFO:
    return "INFO    ";
  case Severity::WARN:
    return "WARNING ";
  case Severity::ERROR:
    return "ERROR   ";
  case Severity::CRITICAL:
    return "CRITICAL";
  default:
    return "NONE    ";
  }
}

class record_t {
public:
  record_t() = default;
  record_t(std::chrono::system_clock::time_point tm_point, Severity severity,
           std::string_view str)
      : tm_point_(tm_point), severity_(severity), tid_(_get_tid()),
        file_str_(str) {
    ss_.reserve(64);
  }
  record_t(record_t &&) = default;
  record_t &operator=(record_t &&) = default;

  Severity get_severity() const { return severity_; }

  const char *get_message() {
    ss_.push_back('\n');
    return ss_.data();
  }

  std::string_view get_file_str() const { return file_str_; }

  unsigned int get_tid() const { return tid_; }

  auto get_time_point() const { return tm_point_; }

  record_t &ref() { return *this; }

  template <typename T> record_t &operator<<(const T &data) {
    using U = remove_cvref_t<T>;
    if constexpr (std::is_floating_point_v<U>) {
      char temp[40];
      const auto end = jkj::dragonbox::to_chars(data, temp);
      ss_.append(temp, std::distance(temp, end));
    } else if constexpr (std::is_same_v<bool, U>) {
      data ? ss_.append("true") : ss_.append("false");
    } else if constexpr (std::is_same_v<char, U>) {
      data ? ss_.append("true") : ss_.append("false");
      ss_.push_back(data);
    } else if constexpr (std::is_enum_v<U>) {
      int val = (int)data;
      *this << val;
    } else if constexpr (std::is_integral_v<U>) {
      char buf[32];
      auto [ptr, ec] = std::to_chars(buf, buf + 32, data);
      ss_.append(buf, std::distance(buf, ptr));
    } else if constexpr (std::is_pointer_v<U>) {
      char buf[32] = {"0x"};
      auto [ptr, ec] = std::to_chars(buf + 2, buf + 32, (uintptr_t)data, 16);
      ss_.append(buf, std::distance(buf, ptr));
    } else if constexpr (std::is_same_v<std::string, U> ||
                         std::is_same_v<std::string_view, U>) {
      ss_.append(data.data(), data.size());
    } else if constexpr (detail::c_array_v<U>) {
      ss_.append(data);
    } else if constexpr (detail::has_data_v<U>) {
      ss_.append(data.data());
    } else if constexpr (detail::has_str_v<U>) {
      ss_.append(data.str());
    } else if constexpr (std::is_same_v<std::chrono::system_clock::time_point,
                                        U>) {
      ss_.append(
          get_local_time_str(std::chrono::system_clock::to_time_t(data)));
    } else {
      std::stringstream ss;
      ss << data;
      ss_.append(ss.str());
    }

    return *this;
  }

  void gen_content() {
    if (args_cache_) {
      ss_.append(args_cache_());
      args_cache_ = {};
    }
  }

  template <typename T> decltype(auto) transform(T &t) {
    using U = std::remove_reference_t<T>;

    if constexpr (std::is_same_v<U, const char *> || fixed_array_v<U> ||
                  string_view_v<U>) {
      return std::string(t);
    } else if constexpr (string_v<U>) {
      return std::move(t);
    } else {
      return t;
    }
  }

  template <typename Tuple, size_t... I>
  auto get_args_impl(Tuple &&tp, std::index_sequence<I...>) {
    std::tuple<> t;
    return std::tuple_cat(t, std::make_tuple(transform(std::get<I>(tp)))...);
  }

  template <FormatType fmt_type, typename... Args>
  record_t &format(bool async, const char *fmt, Args &&...args) {
    size_t len = strlen(fmt);
    std::string str;
    str.reserve(len);
    str.append(fmt);
    args_cache_ = [str = std::move(str),
                   args1 = get_args_impl(
                       std::make_tuple(std::forward<Args>(args)...),
                       std::make_index_sequence<sizeof...(Args)>{})]() mutable {
      return std::apply(
          [str = std::move(str)](auto &&...args) mutable {
            if constexpr (fmt_type == FormatType::FORMAT) {
              return fmt::format(str.data(), std::move(args)...);
            } else {
              return fmt::sprintf(str.data(), std::move(args)...);
            }
          },
          args1);
    };

    if (!async) {
      ss_.append(args_cache_());
      args_cache_ = {};
    }

    return *this;
  }

private:
  template <typename... Args>
  void printf_string_format(const char *fmt, Args &&...args) {
    size_t size = snprintf(nullptr, 0, fmt, std::forward<Args>(args)...);

#ifdef YLT_ENABLE_PMR
#if __has_include(<memory_resource>)
    char arr[1024];
    std::pmr::monotonic_buffer_resource resource(arr, 1024);
    std::pmr::string buf{&resource};
#endif
#else
    std::string buf;
#endif
    buf.reserve(size + 1);
    buf.resize(size);

    snprintf(&buf[0], size + 1, fmt, args...);

    ss_.append(buf);
  }

  unsigned int _get_tid() {
    static thread_local unsigned int tid = get_tid_impl();
    return tid;
  }

  unsigned int get_tid_impl() {
#ifdef _WIN32
    return std::hash<std::thread::id>{}(std::this_thread::get_id());
#elif defined(__linux__)
    return static_cast<unsigned int>(::syscall(__NR_gettid));
#elif defined(__FreeBSD__)
    long tid;
    syscall(SYS_thr_self, &tid);
    return static_cast<unsigned int>(tid);
#elif defined(__rtems__)
    return rtems_task_self();
#elif defined(__APPLE__)
    uint64_t tid64;
    pthread_threadid_np(NULL, &tid64);
    return static_cast<unsigned int>(tid64);
#else
    return 0;
#endif
  }

  std::chrono::system_clock::time_point tm_point_;
  Severity severity_;
  unsigned int tid_;
  std::string file_str_;

#ifdef YLT_ENABLE_PMR
#if __has_include(<memory_resource>)
  char arr_[1024];
  std::pmr::monotonic_buffer_resource resource_;
  std::pmr::string ss_{&resource_};
#endif
#else
  std::string ss_;
#endif
  std::function<std::string()> args_cache_;
};

#define TO_STR(s) #s

#define GET_STRING(filename, line)                                             \
  [] {                                                                         \
    constexpr auto path = refvalue::meta_string{filename};                     \
    constexpr size_t pos =                                                     \
        path.rfind(std::filesystem::path::preferred_separator);                \
    constexpr auto name = path.substr<pos + 1>();                              \
    constexpr auto prefix = name + ":" + TO_STR(line);                         \
    return "[" + prefix + "] ";                                                \
  }()

} // namespace easylog