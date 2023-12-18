#include "utility/Logger.h"

namespace utility {

std::string now() noexcept {
  std::time_t timeNow =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

  std::string timeString = std::ctime(&timeNow);
  timeString.resize(timeString.size() - 1);

  return timeString;
}

} // namespace utility