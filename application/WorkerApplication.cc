#include "application/WorkerApplication.h"

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
  utility::logInfo("Receiving task %u, %s", request->taskid(),
                   request->content());

  {
    std::unique_lock lock(mux_);

    running_task_ids_.insert(request->taskid());
  }

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

} // namespace application