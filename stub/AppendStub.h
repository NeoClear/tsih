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
  AppendStub(const std::vector<std::unique_ptr<RaftService::Stub>>& raftStubs);

  void sendAppendEntriesRequest(
      std::vector<bool> requestFilter, std::vector<ClientContext>& context,
      const std::vector<AppendEntriesArgument>& request,
      std::vector<AppendEntriesResult>& reply, std::vector<bool>& rpcStatus);

private:
  const std::vector<std::unique_ptr<RaftService::Stub>>& raft_stubs_;
};

} // namespace stub
