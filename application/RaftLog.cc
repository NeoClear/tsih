#include "application/RaftLog.h"

namespace application {

SubmitTaskLog::SubmitTaskLog(uint64_t taskId, std::string task)
    : task_id_(taskId), task_(task) {}

void SubmitTaskLog::applyLog(TaskSchedule& schedule) {
  schedule.addTask(task_id_, task_);
}

AssignTaskLog::AssignTaskLog(uint64_t taskId, uint64_t workerIndex)
    : task_id_(taskId), worker_index_(workerIndex) {}

void AssignTaskLog::applyLog(TaskSchedule& schedule) {
  schedule.assignTask(task_id_, worker_index_);
}

TaskFinishLog::TaskFinishLog(uint64_t taskId, bool success)
    : task_id_(taskId), success_(success) {}

void TaskFinishLog::applyLog(TaskSchedule& schedule) {
  schedule.finishTask(task_id_, success_);
}

} // namespace application