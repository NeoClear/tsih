#include "application/RaftApplication.h"
#include "stub/ElectionStub.h"
#include "utility/Logger.h"

namespace application {

// Status
// RaftServiceImpl::AppendEntriesRequest(ServerContext* context,
//                                       const AppendEntriesArgument* request,
//                                       google::protobuf::Empty* reply) {
//   const auto [currentTerm, success] = raft_state_.handleAppendEntries(
//       request->term(), request->leaderid(), request->prevlogindex(),
//       request->prevlogterm(), request->entries(), request->leadercommit());

//   return Status::OK;
// }

// Status RaftServiceImpl::AppendEntriesReply(ServerContext* context,
//                                            const AppendEntriesResult*
//                                            request, google::protobuf::Empty*
//                                            reply) {
//   return Status::OK;
// }

/**
 * @brief Request vote to the current node
 *
 * @param context Misc
 * @param request Request containing vote request information
 * @param reply Vote result
 * @return Status
 */
// Status RaftServiceImpl::RequestVoteRequest(ServerContext* context,
//                                            const RequestVoteArgument*
//                                            request, google::protobuf::Empty*
//                                            reply) {
//   const auto [currentTerm, result] = raft_state_.handleVoteRequest(
//       request->term(), request->candidateid(), request->lastlogindex(),
//       request->lastlogterm());

//   stub::ElectionStub::replyTo(request->candidateid(), currentTerm, result);

//   return Status::OK;
// }

// Status RaftServiceImpl::RequestVoteReply(ServerContext* context,
//                                          const RequestVoteResult* request,
//                                          google::protobuf::Empty* reply) {
//   raft_state_.handleVoteReply(request->term(), request->votegranted());

//   return Status::OK;
// }

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
  raft_state_.handlePingMsg(request->server_identity().server_type(),
                            request->server_identity().server_index());

  ServerUnaryReactor* reactor = context->DefaultReactor();
  reactor->Finish(Status::OK);

  return reactor;
}

// Status RaftServiceImpl::Ping(ServerContext* context, const PingMessage*
// request,
//                              google::protobuf::Empty* reply) {
//   utility::logCrit("Received ping from leader");
//   raft_state_.handlePingMsg(request->server_identity().server_type(),
//                             request->server_identity().server_index());
//   return Status::OK;
// }

// Status RaftServiceImpl::AddTask(ServerContext* context, const Task* request,
//                                 google::protobuf::Empty* reply) {
//   raft_state_.handleAddTask(request->value());
//   return Status::OK;
// }

} // namespace application
