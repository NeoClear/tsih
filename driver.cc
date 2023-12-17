#include "utility/deadliner.h"

#include <future>
#include <iostream>
#include <thread>

int main() {
  std::mutex mux;

  utility::Deadliner deadliner([&mux]() {
    std::unique_lock lock(mux);
    std::cout << "Finished!" << std::endl;
  });

  deadliner.setDeadline(10);
  std::this_thread::sleep_for(std::chrono::milliseconds(5));
  deadliner.setDeadline(10);
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  deadliner.setDeadline(10);
  deadliner.setDeadline(1000);

  std::this_thread::sleep_for(std::chrono::milliseconds(2000));

  deadliner.setDeadline(10);
  uint64_t a;
  return 0;
}
