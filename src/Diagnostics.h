#pragma once

#include <iostream>
#include <ostream>
#include <streambuf>

namespace c_hat {

// Global switch for the compiler's internal debug tracing.  It is off by
// default so normal compilation stays quiet; `chc --verbose` turns it on.
inline bool &debugLogsEnabled() {
  static bool enabled = false;
  return enabled;
}

class NullStreamBuffer : public std::streambuf {
protected:
  int overflow(int c) override { return c; }
};

inline std::ostream &debugStream() {
  static NullStreamBuffer buffer;
  static std::ostream nullStream(&buffer);
  return debugLogsEnabled() ? std::cerr : nullStream;
}

} // namespace c_hat
