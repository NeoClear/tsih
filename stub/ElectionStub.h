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
  ElectionStub(const uint64_t master_count, const uint64_t candidate_idx,
               const uint64_t term, const int64_t last_log_index,
               const uint64_t last_log_term);

  void initiateElection();

  static void replyTo(const uint64_t destIdx, const uint64_t term,
                      const bool voted) {
    token::RequestVoteResult result;

    result.set_term(term);
    result.set_votegranted(voted);

    grpc::ClientContext context;
    google::protobuf::Empty empty;

    std::unique_ptr<api::RaftService::Stub> raftStub =
        RaftService::NewStub(grpc::CreateChannel(
            absl::StrFormat("localhost:%u", destIdx + MASTER_BASE_PORT),
            grpc::InsecureChannelCredentials()));
    raftStub->RequestVoteReply(&context, result, &empty);
  }

private:
  std::vector<std::unique_ptr<RaftService::Stub>> raft_stubs_;

  const uint64_t candidate_idx_;
  const uint64_t term_;
  const int64_t last_log_index_;
  const uint64_t last_log_term_;
  const uint64_t raft_size_;
};

} // namespace stub
