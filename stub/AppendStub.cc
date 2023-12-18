#include "stub/AppendStub.h"

namespace stub {

AppendStub::AppendStub(uint64_t raftSize) {
  for (uint64_t port = MASTER_BASE_PORT; port < MASTER_BASE_PORT + raftSize;
       ++port) {
    std::shared_ptr<Channel> channel =
        grpc::CreateChannel(absl::StrFormat("0.0.0.0:%u", port),
                            grpc::InsecureChannelCredentials());
    raft_stubs_.emplace_back(RaftService::NewStub(channel));
  }
}

void AppendStub::sendAppendEntriesRequest(
    uint64_t term, uint64_t leaderId, int64_t prevLogIndex,
    uint64_t prevLogTerm, const std::vector<token::LogEntry>& entries,
    uint64_t leaderCommit, uint64_t destinationId) {
  token::AppendEntriesArgument request;

  request.set_term(term);
  request.set_leaderid(leaderId);
  request.set_prevlogindex(prevLogIndex);
  request.set_prevlogterm(prevLogTerm);
  request.set_leadercommit(leaderCommit);

  for (const token::LogEntry& entry : entries) {
    token::LogEntry* entryIt = request.add_entries();
    entryIt->CopyFrom(entry);
  }

  google::protobuf::Empty empty;
  grpc::ClientContext context;

  raft_stubs_[destinationId]->AppendEntriesRequest(&context, request, &empty);
}

} // namespace stub
