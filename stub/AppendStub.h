#pragma once

#include <future>
#include <grpcpp/grpcpp.h>
#include <vector>

#include "absl/container/flat_hash_set.h"

#include "proto/api.grpc.pb.h"

#include "config.h"

using api::RaftService;
using token::AppendEntriesArgument;
using token::AppendEntriesResult;
using token::PingMessage;
using token::ServerIdentity;

using grpc::Channel;
using grpc::ClientContext;

namespace stub {
class AppendStub {
public:
  AppendStub(uint64_t raftSize);

  // The actual request
  // uint64_t term, uint64_t leaderId,
  //                             int64_t prevLogIndex, uint64_t prevLogTerm,
  //                             const std::vector<token::LogEntry>&
  //                             entries, uint64_t leaderCommit,
  void sendAppendEntriesRequest(
      std::vector<bool> requestFilter, std::vector<ClientContext>& context,
      const std::vector<AppendEntriesArgument>& request,
      std::vector<AppendEntriesResult>& reply, std::vector<bool>& rpcStatus);

private:
  std::vector<std::unique_ptr<RaftService::Stub>> raft_stubs_;
};

} // namespace stub
