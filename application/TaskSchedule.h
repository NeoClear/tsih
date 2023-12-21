#pragma once

#include <inttypes.h>
#include <mutex>
#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"

#include "proto/api.grpc.pb.h"

#include "utility/Deadliner.h"

namespace application {

class TaskSchedule {
public:
  explicit TaskSchedule();

  void addTask(uint64_t taskId, std::string task);
  void assignTask(uint64_t taskId, uint64_t workerIndex);
  void finishTask(uint64_t taskId, bool success);

  token::TaskStatus queryTaskStatus(uint64_t taskId);

  uint64_t getPendingTasks();
  uint64_t getRunningTasks();
  uint64_t getFinishedTasks();
  uint64_t getWorkerCount();

  /**
   * @brief Return null when no suitable worker is available
   */
  std::optional<uint64_t> findSuitableWorker();

  void workerPing(uint64_t workerIndex);

private:
  std::mutex mux_;

  // TaskId -> TaskContent
  absl::flat_hash_map<uint64_t, std::string> task_info_;

  absl::flat_hash_set<uint64_t> pending_tasks_;
  // TaskId -> WorkerIndex
  absl::flat_hash_map<uint64_t, uint64_t> running_tasks_;
  absl::flat_hash_map<uint64_t, bool> finished_tasks_;

  // Worker to liveness
  // in range [0, 1]
  absl::flat_hash_set<uint64_t> living_workers_;
  // Worker that pinged from previous period
  absl::flat_hash_set<uint64_t> pinged_workers_;

  absl::flat_hash_map<uint64_t, uint64_t> worker_load_;

  utility::Deadliner worker_timeout_;
  void workerTimeoutCallback();
};

} // namespace application