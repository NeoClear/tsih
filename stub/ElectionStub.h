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
class ElectionStub {
public:
  ElectionStub(const uint64_t raftSize, const uint64_t candidateIdx);

  /**
   * @brief Blocking function that returns the voting result
   *
   * Any failed communication would be considered as rejection
   */
  std::pair<bool, uint64_t> elect(const uint64_t term,
                                  const int64_t lastLogIndex,
                                  const uint64_t lastLogTerm);

private:
  std::vector<std::unique_ptr<RaftService::Stub>> raft_stubs_;

  const uint64_t candidate_idx_;
};

} // namespace stub
