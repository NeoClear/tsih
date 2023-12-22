#include "ExecuteTaskStub.h"

#include <latch>

#include "utility/Logger.h"

#include "config.h"

namespace stub::ExecuteTaskStub {

bool executeTask(uint64_t workerIndex, uint64_t taskId,
                 std::string taskContent) {
  std::shared_ptr<Channel> channel = grpc::CreateChannel(
      absl::StrFormat("0.0.0.0:%u", workerIndex + WORKER_BASE_PORT),
      grpc::InsecureChannelCredentials());
  std::unique_ptr<api::WorkerService::Stub> stub =
      api::WorkerService::NewStub(channel);

  std::latch executeTaskFinished(1);

  grpc::ClientContext context;
  token::ExecuteTaskRequest request;
  token::ExecuteTaskReply reply;

  request.set_taskid(taskId);
  request.set_content(taskContent);

  utility::logInfo("ExecTask stub called");

  stub->async()->ExecuteTask(&context, &request, &reply,
                             [&executeTaskFinished](grpc::Status status) {
                               executeTaskFinished.count_down();
                             });

  executeTaskFinished.wait();

  return true;
}

} // namespace stub::ExecuteTaskStub