#pragma once

#include <grpcpp/grpcpp.h>

#include "absl/container/flat_hash_set.h"

#include "proto/api.grpc.pb.h"

using api::RaftService;
using token::PingMessage;
using token::ServerIdentity;

using grpc::Channel;

namespace stub {
class PingStub {
public:
  PingStub(const std::vector<std::unique_ptr<RaftService::Stub>>& raftStubs,
           token::ServerType type, uint64_t candidateIdx);

  void ping();

private:
  const std::vector<std::unique_ptr<RaftService::Stub>>& raft_stubs_;

  const token::ServerType type_;
  const uint64_t candidate_idx_;
};

} // namespace stub
