#pragma once

#include <inttypes.h>
#include <string>

#include "application/TaskSchedule.h"

namespace application {

class RaftLog {
public:
  /**
   * @brief Apply log to schedule, implementation should modify it to satisfy
   * the need
   */
  virtual void applyLog(TaskSchedule& schedule) = 0;
};

class SubmitTaskLog : public RaftLog {
public:
  explicit SubmitTaskLog(uint64_t taskId, std::string task);

  void applyLog(TaskSchedule& schedule) override;

private:
  uint64_t task_id_;
  std::string task_;
};

class AssignTaskLog : public RaftLog {
public:
  explicit AssignTaskLog(uint64_t taskId, uint64_t workerIndex);

  void applyLog(TaskSchedule& schedule) override;

private:
  uint64_t task_id_;
  uint64_t worker_index_;
};

class TaskFinishLog : public RaftLog {
public:
  explicit TaskFinishLog(uint64_t taskId, bool success);

  void applyLog(TaskSchedule& schedule) override;

private:
  uint64_t task_id_;
  bool success_;
};

} // namespace application