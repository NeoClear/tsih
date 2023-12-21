#include "application/WorkerApplication.h"

using grpc::Status;

namespace application {

ServerUnaryReactor*
WorkerServiceImpl::ExecuteTask(CallbackServerContext* context,
                               const ExecuteTaskRequest* request,
                               ExecuteTaskReply* reply) {
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

} // namespace application