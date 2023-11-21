#pragma once

#include <functional>
#include <thread>

class Periodic {
protected:
  /**
   * @brief Used within class to setup periodic tasks, thus protected
   */
  void addPeriodicTask(std::function<void()> task, const uint64_t millsecond) {
    periodicTasks.emplace_back(std::make_shared<std::function<void()>>(task),
                               millsecond);
  }

public:
  /**
   * @brief Called outside of the class to initiate all tasks, thus public
   */
  void runPeriodicTasks() {
    for (const auto &[task, millsecond] : periodicTasks) {
      const auto taskRunner = [](std::shared_ptr<std::function<void()>> task,
                                 const uint64_t millsecond) {
        for (;;) {
          (*task)();
          std::this_thread::sleep_for(std::chrono::milliseconds(millsecond));
        }
      };

      periodicThreads.emplace_back(taskRunner, task, millsecond);
    }
  }

  ~Periodic() {
    for (std::thread &periodic : periodicThreads) {
      if (periodic.joinable()) {
        periodic.join();
      }
    }
  }

private:
  std::vector<std::pair<std::shared_ptr<std::function<void()>>, uint64_t>>
      periodicTasks;

  std::vector<std::thread> periodicThreads;
};
