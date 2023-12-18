#include "stub/ElectionStub.h"

#include "absl/container/flat_hash_set.h"

#include "utility/Logger.h"

#include "config.h"

using grpc::ClientContext;
using token::PingMessage;

namespace stub {

ElectionStub::ElectionStub(const uint64_t master_count,
                           const uint64_t candidate_idx, const uint64_t term,
                           const int64_t last_log_index,
                           const uint64_t last_log_term)
    : candidate_idx_(candidate_idx), term_(term),
      last_log_index_(last_log_index), last_log_term_(last_log_term),
      raft_size_(master_count) {}

void ElectionStub::initiateElection() {
  token::RequestVoteArgument request;

  request.set_term(term_);
  request.set_candidateid(candidate_idx_);
  request.set_lastlogindex(last_log_index_);
  request.set_lastlogterm(last_log_term_);

  for (uint64_t i = 0; i < raft_size_; ++i) {
    if (i == candidate_idx_) {
      continue;
    }

    ClientContext context;
    google::protobuf::Empty empty;

    std::shared_ptr<Channel> channel =
        grpc::CreateChannel(absl::StrFormat("0.0.0.0:%u", i + MASTER_BASE_PORT),
                            grpc::InsecureChannelCredentials());
    std::unique_ptr<api::RaftService::Stub> stub =
        api::RaftService::NewStub(channel);
    grpc::Status status = stub->RequestVoteRequest(&context, request, &empty);

    // utility::logInfo("Status: %u", status.ok());
  }
}

} // namespace stub
