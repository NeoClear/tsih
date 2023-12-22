#include "application/WorkerApplication.h"

#include <latch>

using grpc::Status;

namespace application {

WorkerServiceImpl::WorkerServiceImpl(uint64_t raftSize, uint64_t workerIndex)
    : raft_stubs_(initRaftStubs(raftSize)),
      ping_stub_(raft_stubs_, token::ServerType::WORKER, workerIndex),
      ping_timeout_(std::bind(&WorkerServiceImpl::pingTimeoutCallback, this)) {
  ping_timeout_.setDeadline(100);
}

std::vector<std::unique_ptr<RaftService::Stub>>
WorkerServiceImpl::initRaftStubs(uint64_t raftSize) {
  std::vector<std::unique_ptr<RaftService::Stub>> stubs;

  for (uint64_t port = MASTER_BASE_PORT; port < MASTER_BASE_PORT + raftSize;
       ++port) {
    std::shared_ptr<Channel> channel =
        grpc::CreateChannel(absl::StrFormat("0.0.0.0:%u", port),
                            grpc::InsecureChannelCredentials());
    stubs.emplace_back(RaftService::NewStub(channel));
  }

  return stubs;
}

ServerUnaryReactor*
WorkerServiceImpl::ExecuteTask(CallbackServerContext* context,
                               const ExecuteTaskRequest* request,
                               ExecuteTaskReply* reply) {
  std::thread taskExeutionThread(std::bind(&WorkerServiceImpl::taskExecutor,
                                           this, request->taskid(),
                                           request->content()));
  taskExeutionThread.detach();

  ServerUnaryReactor* reactor = context->DefaultReactor();
  reactor->Finish(Status::OK);

  return reactor;
}

ServerUnaryReactor*
WorkerServiceImpl::QueryWorkerStatus(CallbackServerContext* context,
                                     const google::protobuf::Empty* request,
                                     QueryWorkerStatusReply* reply) {
  reply->set_runningjobs(4);

  ServerUnaryReactor* reactor = context->DefaultReactor();
  reactor->Finish(Status::OK);

  return reactor;
}

ServerUnaryReactor*
WorkerServiceImpl::QueryTaskStatus(CallbackServerContext* context,
                                   const QueryTaskStatusRequest* request,
                                   QueryTaskStatusReply* reply) {
  reply->set_taskstatus(token::TaskStatus::RUNNING);

  ServerUnaryReactor* reactor = context->DefaultReactor();
  reactor->Finish(Status::OK);

  return reactor;
}

void WorkerServiceImpl::pingTimeoutCallback() {
  std::vector<uint64_t> runningTasks;

  {
    std::unique_lock lock(mux_);

    runningTasks.insert(runningTasks.cend(), running_task_ids_.begin(),
                        running_task_ids_.end());
  }

  ping_stub_.ping(runningTasks);

  ping_timeout_.setDeadline(100);
}

void WorkerServiceImpl::taskExecutor(uint64_t taskId, std::string taskContent) {
  {
    std::unique_lock lock(mux_);

    if (running_task_ids_.count(taskId)) {
      return;
    }

    running_task_ids_.insert(taskId);
  }

  utility::logInfo("Executing task (%u, %s)", taskId, taskContent);

  // Run the script
  int r = system(absl::StrFormat("./task/%s 2>&1 > ./output/task_%u.txt",
                                 taskContent, taskId)
                     .c_str());

  utility::logInfo("Finished task (%u, %s)", taskId, taskContent);

  {
    std::unique_lock lock(mux_);

    assert(running_task_ids_.count(taskId));

    running_task_ids_.erase(taskId);
  }

  // Send completion message to raft group
  bool finishSuccess = finishTask(taskId, r == 0);

  if (finishSuccess) {
    utility::logInfo("Job status updated (%u, %s)", taskId,
                     r == 0 ? "success" : "fail");
  } else {
    utility::logInfo("Unable to contact masters");
  }
}

bool WorkerServiceImpl::finishTask(uint64_t taskId, bool success) {
  token::FinishTaskRequest request;

  request.set_taskid(taskId);
  request.set_success(success);

  for (uint64_t i = 0; i < raft_stubs_.size(); ++i) {
    grpc::ClientContext context;
    token::FinishTaskReply reply;

    std::latch onRequestCompleted(1);
    bool invoked = false;

    raft_stubs_[i]->async()->FinishTask(
        &context, &request, &reply,
        [&onRequestCompleted, &invoked](Status status) {
          if (status.ok()) {
            invoked = true;
          }
          onRequestCompleted.count_down();
        });

    onRequestCompleted.wait();

    if (invoked && reply.success()) {
      return true;
    }
  }

  return false;
}

} // namespace application