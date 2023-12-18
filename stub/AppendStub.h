#pragma once

#include <grpcpp/grpcpp.h>
#include <vector>

#include "absl/container/flat_hash_set.h"

#include "proto/api.grpc.pb.h"

#include "config.h"

using api::RaftService;
using token::PingMessage;
using token::ServerIdentity;

using grpc::Channel;

namespace stub {
class AppendStub {
public:
  AppendStub(uint64_t raftSize);

  void sendAppendEntriesRequest(uint64_t term, uint64_t leaderId,
                                int64_t prevLogIndex, uint64_t prevLogTerm,
                                const std::vector<token::LogEntry>& entries,
                                uint64_t leaderCommit, uint64_t destinationId);

private:
  std::vector<std::unique_ptr<RaftService::Stub>> raft_stubs_;
};

} // namespace stub
