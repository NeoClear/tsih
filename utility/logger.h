#pragma once

#include <chrono>

#include "absl/strings/str_format.h"
#include <string_view>

namespace utility {

using namespace std::literals;

enum class LOG_TYPE {
  INFO,
  ERR,
  CRIT,
};

constexpr std::string_view logType2StringView(const LOG_TYPE logType) {
  switch (logType) {
  case LOG_TYPE::INFO:
    return "\u001b[32;1mINFO\u001b[0m"sv;
  case LOG_TYPE::ERR:
    return "\u001b[31;1mERR\u001b[0m"sv;
  case LOG_TYPE::CRIT:
    return "\u001b[31;1mCRIT!!!\u001b[0m"sv;
  default:
    return "\u001b[33;1mUNDEFINED\u001b[0m"sv;
  }
}

std::string now() noexcept;

template <typename... Args>
void inline logInfo(const LOG_TYPE logType,
                    const absl::FormatSpec<Args...> &formatString,
                    const Args &...args) {
  absl::FPrintF(stderr, "%s [%s]: %s\n", logType2StringView(logType), now(),
                absl::StrFormat(formatString, args...));
}

template <typename... Args>
void inline logInfo(const absl::FormatSpec<Args...> &formatString,
                    const Args &...args) {
  absl::FPrintF(stderr, "%s [%s]: %s\n", logType2StringView(LOG_TYPE::INFO),
                now(), absl::StrFormat(formatString, args...));
}

template <typename... Args>
void inline logError(const absl::FormatSpec<Args...> &formatString,
                     const Args &...args) {
  absl::FPrintF(stderr, "%s [%s]: %s\n", logType2StringView(LOG_TYPE::ERR),
                now(), absl::StrFormat(formatString, args...));
}

} // namespace utility
