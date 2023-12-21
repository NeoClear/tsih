#include "application/TaskSchedule.h"
#include "utility/Logger.h"

namespace application {

TaskSchedule::TaskSchedule()
    : worker_timeout_(std::bind(&TaskSchedule::workerTimeoutCallback, this)) {
  worker_timeout_.setDeadline(1000);
}

void TaskSchedule::addTask(uint64_t taskId, std::string task) {
  std::unique_lock lock(mux_);

  task_info_[taskId] = task;
  pending_tasks_.insert(taskId);

  utility::logCrit("Task %u added", taskId);
}

void TaskSchedule::assignTask(uint64_t taskId, uint64_t workerIndex) {
  std::unique_lock lock(mux_);

  assert(task_info_.count(taskId));
  assert(pending_tasks_.count(taskId));

  pending_tasks_.erase(taskId);
  running_tasks_[taskId] = workerIndex;
}

void TaskSchedule::finishTask(uint64_t taskId, bool success) {
  std::unique_lock lock(mux_);

  assert(pending_tasks_.count(taskId));

  finished_tasks_[taskId] = success;
}

token::TaskStatus TaskSchedule::queryTaskStatus(uint64_t taskId) {
  std::unique_lock lock(mux_);

  if (pending_tasks_.count(taskId)) {
    return token::TaskStatus::PENDING;
  }

  if (running_tasks_.count(taskId)) {
    return token::TaskStatus::RUNNING;
  }

  if (finished_tasks_.count(taskId)) {
    return finished_tasks_[taskId] ? token::TaskStatus::SUCCEEDED
                                   : token::TaskStatus::FAILED;
  }

  return token::TaskStatus::UNKNOWN;
}

uint64_t TaskSchedule::getPendingTasks() {
  std::unique_lock lock(mux_);
  return pending_tasks_.size();
}

uint64_t TaskSchedule::getRunningTasks() {
  std::unique_lock lock(mux_);
  return running_tasks_.size();
}

uint64_t TaskSchedule::getFinishedTasks() {
  std::unique_lock lock(mux_);
  return finished_tasks_.size();
}

uint64_t TaskSchedule::getWorkerCount() {
  std::unique_lock lock(mux_);
  return living_workers_.size();
}

std::optional<uint64_t> TaskSchedule::findSuitableWorker() {
  std::unique_lock lock(mux_);

  // Go through living worker with smallest load

  std::optional<uint64_t> selectedWorker;

  for (uint64_t workerIndex : living_workers_) {
    if (!selectedWorker.has_value()) {
      selectedWorker = workerIndex;
    } else {
      auto selectedWorkerIt = worker_load_.find(*selectedWorker);
      auto targetWorkerIt = worker_load_.find(workerIndex);

      if (selectedWorkerIt != worker_load_.cend() &&
          targetWorkerIt != worker_load_.cend() &&
          targetWorkerIt->second < selectedWorkerIt->second) {
        selectedWorker = workerIndex;
      }
    }
  }

  return selectedWorker;
}

void TaskSchedule::workerPing(uint64_t workerIndex) {
  std::unique_lock lock(mux_);

  pinged_workers_.insert(workerIndex);
}

void TaskSchedule::workerTimeoutCallback() {
  std::unique_lock lock(mux_);

  living_workers_.swap(pinged_workers_);
  pinged_workers_.clear();

  // Liveness check for worker is 1000
  worker_timeout_.setDeadline(1000);
}

} // namespace application