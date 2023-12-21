#include "application/RaftApplication.h"
#include "stub/ElectionStub.h"
#include "utility/Logger.h"

namespace application {

ServerUnaryReactor*
RaftServiceImpl::AppendEntries(CallbackServerContext* context,
                               const AppendEntriesArgument* request,
                               AppendEntriesResult* reply) {
  const auto [currentTerm, success] = raft_state_.handleAppendEntries(
      request->term(), request->leaderid(), request->prevlogindex(),
      request->prevlogterm(), request->entries(), request->leadercommit());

  reply->set_term(currentTerm);
  reply->set_success(success);

  ServerUnaryReactor* reactor = context->DefaultReactor();
  reactor->Finish(Status::OK);

  return reactor;
}

ServerUnaryReactor*
RaftServiceImpl::RequestVote(CallbackServerContext* context,
                             const RequestVoteArgument* request,
                             RequestVoteResult* reply) {
  const auto [currentTerm, result] = raft_state_.handleVoteRequest(
      request->term(), request->candidateid(), request->lastlogindex(),
      request->lastlogterm());

  utility::logInfo("Raft state: %s", raft_state_.toString());
  utility::logInfo("Responded: %u, %s", currentTerm, result ? "yes" : "no");

  reply->set_term(currentTerm);
  reply->set_votegranted(result);

  ServerUnaryReactor* reactor = context->DefaultReactor();
  reactor->Finish(Status::OK);

  return reactor;
}

ServerUnaryReactor* RaftServiceImpl::Ping(CallbackServerContext* context,
                                          const PingMessage* request,
                                          google::protobuf::Empty* reply) {
  raft_state_.handlePing(request->server_identity().server_type(),
                         request->server_identity().server_index());

  ServerUnaryReactor* reactor = context->DefaultReactor();
  reactor->Finish(Status::OK);

  return reactor;
}

ServerUnaryReactor*
RaftServiceImpl::SubmitTask(CallbackServerContext* context,
                            const SubmitTaskRequest* request,
                            SubmitTaskReply* reply) {
  std::future<std::pair<bool, uint64_t>> futureSuccess =
      raft_state_.handleTaskSubmission(request->value());

  auto result = futureSuccess.get();

  reply->set_success(result.first);
  reply->set_jobid(result.second);

  ServerUnaryReactor* reactor = context->DefaultReactor();
  reactor->Finish(Status::OK);

  return reactor;
}

ServerUnaryReactor*
RaftServiceImpl::QueryTask(CallbackServerContext* context,
                           const QueryTaskStatusRequest* request,
                           QueryTaskStatusReply* reply) {

  reply->set_taskstatus(raft_state_.handleTaskQuery(request->taskid()));

  ServerUnaryReactor* reactor = context->DefaultReactor();
  reactor->Finish(Status::OK);

  return reactor;
}

ServerUnaryReactor*
RaftServiceImpl::QueryService(CallbackServerContext* context,
                              const QueryServiceRequest* request,
                              Count* reply) {
  reply->set_count(raft_state_.handleServiceQuery(request->taskstatus()));

  ServerUnaryReactor* reactor = context->DefaultReactor();
  reactor->Finish(Status::OK);

  return reactor;
}

} // namespace application
