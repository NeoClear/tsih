#pragma once

#include <functional>
#include <thread>

namespace utility {

class Deadliner {
public:
  explicit Deadliner(const std::function<void()>& fn);

  void setDeadline(uint64_t millsecond);
  void setRandomDeadline(uint64_t lowerBound = 0, uint64_t upperBound = 0);

  ~Deadliner();

private:
  bool pending_;
  std::unique_ptr<std::function<void()>> task_;
  uint64_t deadline_;

  std::thread notifier_;
  std::mutex mux_;

  void notifyThread();
};

} // namespace utility