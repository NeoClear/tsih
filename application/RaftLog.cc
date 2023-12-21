#include "application/RaftLog.h"

namespace application {

std::unique_ptr<RaftLog>
RaftLog::buildRaftLog(token::TaskActionEntry actionEntry) {
  if (actionEntry.has_submittaskentry()) {
    return std::make_unique<SubmitTaskLog>(
        actionEntry.submittaskentry().value());
  }

  if (actionEntry.has_assigntaskentry()) {
    return std::make_unique<AssignTaskLog>(
        actionEntry.assigntaskentry().taskid(),
        actionEntry.assigntaskentry().workerindex());
  }

  if (actionEntry.has_finishtaskentry()) {
    return std::make_unique<FinishTaskLog>(
        actionEntry.finishtaskentry().taskid(),
        actionEntry.finishtaskentry().success());
  }

  throw std::runtime_error("Unreachable");
}

RaftLog::~RaftLog() {}

SubmitTaskLog::SubmitTaskLog(std::string task) : task_(task) {}

void SubmitTaskLog::applyLog(TaskSchedule& schedule, uint64_t index) {
  schedule.addTask(index, task_);
}

token::TaskActionEntry SubmitTaskLog::buildLogEntry() {
  token::TaskActionEntry actionEntry;
  actionEntry.mutable_submittaskentry()->set_value(task_);

  return actionEntry;
}

SubmitTaskLog::~SubmitTaskLog() {}

AssignTaskLog::AssignTaskLog(uint64_t taskId, uint64_t workerIndex)
    : task_id_(taskId), worker_index_(workerIndex) {}

void AssignTaskLog::applyLog(TaskSchedule& schedule, uint64_t index) {
  schedule.assignTask(task_id_, worker_index_);
}

token::TaskActionEntry AssignTaskLog::buildLogEntry() {
  token::TaskActionEntry actionEntry;
  actionEntry.mutable_assigntaskentry()->set_taskid(task_id_);
  actionEntry.mutable_assigntaskentry()->set_workerindex(worker_index_);

  return actionEntry;
}

AssignTaskLog::~AssignTaskLog() {}

FinishTaskLog::FinishTaskLog(uint64_t taskId, bool success)
    : task_id_(taskId), success_(success) {}

void FinishTaskLog::applyLog(TaskSchedule& schedule, uint64_t index) {
  schedule.finishTask(task_id_, success_);
}

token::TaskActionEntry FinishTaskLog::buildLogEntry() {
  token::TaskActionEntry actionEntry;
  actionEntry.mutable_finishtaskentry()->set_taskid(task_id_);
  actionEntry.mutable_finishtaskentry()->set_success(success_);

  return actionEntry;
}

FinishTaskLog::~FinishTaskLog() {}

} // namespace application