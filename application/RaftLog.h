#pragma once

#include <inttypes.h>
#include <string>

#include "application/TaskSchedule.h"

#include "proto/api.grpc.pb.h"

namespace application {

class RaftLog {
public:
  /**
   * @brief Apply log to schedule, implementation should modify it to satisfy
   * the need
   */
  virtual void applyLog(TaskSchedule& schedule, uint64_t index) = 0;

  virtual token::TaskActionEntry buildLogEntry() = 0;

  static std::unique_ptr<RaftLog>
  buildRaftLog(token::TaskActionEntry actionEntry);

  virtual ~RaftLog();
};

class SubmitTaskLog : public RaftLog {
public:
  explicit SubmitTaskLog(std::string task);

  void applyLog(TaskSchedule& schedule, uint64_t index) override;
  token::TaskActionEntry buildLogEntry() override;

  ~SubmitTaskLog() override;

private:
  std::string task_;
};

class AssignTaskLog : public RaftLog {
public:
  explicit AssignTaskLog(uint64_t taskId, uint64_t workerIndex);

  void applyLog(TaskSchedule& schedule, uint64_t index) override;
  token::TaskActionEntry buildLogEntry() override;

  ~AssignTaskLog() override;

private:
  uint64_t task_id_;
  uint64_t worker_index_;
};

class FinishTaskLog : public RaftLog {
public:
  explicit FinishTaskLog(uint64_t taskId, bool success);

  void applyLog(TaskSchedule& schedule, uint64_t index) override;
  token::TaskActionEntry buildLogEntry() override;

  ~FinishTaskLog() override;

private:
  uint64_t task_id_;
  bool success_;
};

} // namespace application