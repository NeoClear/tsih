#include "utility/deadliner.h"

#include <random>

namespace utility {

Deadliner::Deadliner(const std::function<void()> &fn)
    : pending_(false), task_(std::make_unique<std::function<void()>>(fn)),
      deadline_(0ull), notifier_([]() {}) {}

void Deadliner::setDeadline(uint64_t millsecond) {
  std::unique_lock lock(mux_);

  uint64_t newDeadline =
      millsecond + std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::steady_clock::now().time_since_epoch())
                       .count();

  if (!pending_) {
    // No work running at the moment
    // notifier_.join();
    notifier_.detach();

    notifier_ = std::thread(std::bind(&Deadliner::notifyThread, this));

    deadline_ = newDeadline;

    pending_ = true;
  } else {
    // Work already running, update the deadline
    deadline_ = newDeadline;
  }
}

void Deadliner::setRandomDeadline(uint64_t lowerBound, uint64_t upperBound) {
  std::random_device randDev;
  std::mt19937 rng(randDev());
  std::uniform_int_distribution<std::mt19937::result_type> distribution(
      lowerBound, upperBound);

  setDeadline(distribution(rng));
}

void Deadliner::notifyThread() {
  for (;;) {

    uint64_t durationDiff;

    {
      std::unique_lock lock(mux_);

      uint64_t currentTime =
          std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::steady_clock::now().time_since_epoch())
              .count();

      durationDiff = currentTime <= deadline_ ? deadline_ - currentTime : 0;

      // We have finished the job
      if (durationDiff == 0) {
        pending_ = false;
      }
    }

    if (durationDiff == 0) {
      (*task_)();
      return;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(durationDiff));
  }
}

Deadliner::~Deadliner() { notifier_.join(); }

} // namespace utility
