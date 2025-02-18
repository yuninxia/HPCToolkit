// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

#ifndef HPCTOOLKIT_PROFILE_UTIL_LOG_H
#define HPCTOOLKIT_PROFILE_UTIL_LOG_H

#include <bitset>
#include <sstream>
#include <string>

namespace hpctoolkit::util::log {

namespace detail {
// Private common base for all message buffers.
class MessageBuffer : public std::ostream {
protected:
  // Subclasses should mess with these as needed.
  MessageBuffer(bool enabled);
  ~MessageBuffer() = default;

public:
  // Messages are movable but not copyable.
  MessageBuffer(MessageBuffer&&);
  MessageBuffer(const MessageBuffer&) = delete;
  MessageBuffer& operator=(MessageBuffer&&);
  MessageBuffer& operator=(const MessageBuffer&) = delete;

  // Attempts to output are disabled if the entire Message is disabled.
  template<class A>
  std::ostream& operator<<(A a) {
    return enabled ? (*(std::ostream*)this << std::forward<A>(a)) : *this;
  }

protected:
  bool enabled;
  std::stringbuf sbuf;

  // Check whether this message is empty or not (i.e. has been moved from)
  bool empty() noexcept;
};
}

/// Structure for logging settings. Makes it easier to change and set up.
class Settings final {
public:
  ~Settings() = default;

  struct all_t {};
  static inline constexpr all_t all = {};
  struct none_t {};
  static inline constexpr none_t none = {};

  /// Initialize with all or none of the message types enabled
  Settings(all_t) : bits(~decltype(bits)(0)) {};
  Settings(none_t) : bits(0) {};

  /// Full initialization
  Settings(bool v_error, bool v_warning, bool v_verbose, bool v_info, bool v_debug)
    : bits((v_error ? 1 : 0) | (v_warning ? 2 : 0) | (v_verbose ? 4 : 0)
           | (v_info ? 8 : 0) | (v_debug ? 16 : 0)) {}

  /// Bitwise operators
  friend Settings operator&(Settings a, Settings b) noexcept { return a.bits & b.bits; }
  Settings& operator&=(Settings o) noexcept { bits &= o.bits; return *this; }
  friend Settings operator|(Settings a, Settings b) noexcept { return a.bits | b.bits; }
  Settings& operator|=(Settings o) noexcept { bits |= o.bits; return *this; }
  friend Settings operator^(Settings a, Settings b) noexcept { return a.bits ^ b.bits; }
  Settings& operator^=(Settings o) noexcept { bits ^= o.bits; return *this; }
  friend Settings operator~(Settings a) noexcept { return ~a.bits; }

  /// Get or set the current Settings used for logging.
  /// Note that set() must be called before using any of the logging facilities.
  // MT: Externally Synchronized (global)
  static Settings get();
  static void set(Settings);

  /// Named versions of the individual bits
  auto error() { return bits[0]; }
  auto warning() { return bits[1]; }
  auto verbose() { return bits[2]; }
  auto info() { return bits[3]; }
  auto debug() { return bits[4]; }

private:
  Settings(std::bitset<5> bits) : bits(bits) {};

  std::bitset<5> bits;
};

/// Fatal error message buffer. When destructed, the program is terminated after
/// the message is written. For cases where things have gone horribly wrong.
/// These will always be printed and cannot be disabled.
struct fatal final : public detail::MessageBuffer {
  fatal();
  [[noreturn]] ~fatal();

  fatal(fatal&&) = delete;
  fatal(const fatal&) = delete;
  fatal& operator=(fatal&&) = default;
  fatal& operator=(const fatal&) = delete;
};

/// Non-fatal error message buffer. For cases where things probably aren't going
/// to plan, but its not fatal enough to crash... yet.
struct error final : public detail::MessageBuffer {
  error();
  ~error();

  error(error&&) = delete;
  error(const error&) = delete;
  error& operator=(error&&) = default;
  error& operator=(const error&) = delete;
};

/// Verbose non-fatal error message buffer. For cases where things aren't ideal,
/// but also not bad enough to be too loud about it.
struct verror final : public detail::MessageBuffer {
  verror();
  ~verror();

  verror(verror&&) = delete;
  verror(const verror&) = delete;
  verror& operator=(verror&&) = default;
  verror& operator=(const verror&) = delete;
};

/// Warning message buffer. For cases where something went wrong, but it should
/// all work out in the end. Probably.
struct warning final : public detail::MessageBuffer {
  warning();
  ~warning();

  warning(warning&&) = delete;
  warning(const warning&) = delete;
  warning& operator=(warning&&) = default;
  warning& operator=(const warning&) = delete;
};

/// Verbose warning message buffer. For cases where something went wrong, but it
/// usually works out in the end, and we don't want to be too loud about it.
struct vwarning final : public detail::MessageBuffer {
  vwarning();
  ~vwarning();

  vwarning(vwarning&&) = delete;
  vwarning(const vwarning&) = delete;
  vwarning& operator=(vwarning&&) = default;
  vwarning& operator=(const vwarning&) = delete;
};

/// Args-info message buffer. To describe how the arguments were interpreted in
/// context. Always output to explain what's happening to the user.
struct argsinfo final : public detail::MessageBuffer {
  argsinfo();
  ~argsinfo();

  argsinfo(argsinfo&&) = delete;
  argsinfo(const argsinfo&) = delete;
  argsinfo& operator=(argsinfo&&) = default;
  argsinfo& operator=(const argsinfo&) = delete;
};

/// Info message buffer. To describe what's going on when its going on, for
/// the curious observer. Not for developers, that's what debug is for.
struct info final : public detail::MessageBuffer {
  info();
  ~info();

  info(info&&) = delete;
  info(const info&) = delete;
  info& operator=(info&&) = default;
  info& operator=(const info&) = delete;
};

/// Debug message buffer. To give as much data as possible about what's
/// happening when its happening. For developers.
struct debug final : public detail::MessageBuffer {
  debug();
  ~debug();

  debug(debug&&) = delete;
  debug(const debug&) = delete;
  debug& operator=(debug&&) = default;
  debug& operator=(const debug&) = delete;
};

}

#endif  // HPCTOOLKIT_PROFILE_UTIL_LOG_H
