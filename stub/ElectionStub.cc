#include "stub/ElectionStub.h"

#include "absl/container/flat_hash_set.h"

#include "utility/Logger.h"

#include "config.h"

using grpc::ClientContext;
using token::PingMessage;

namespace stub {

ElectionStub::ElectionStub(
    const std::vector<std::unique_ptr<RaftService::Stub>>& raftStubs,
    uint64_t candidateIdx)
    : raft_stubs_(raftStubs), candidate_idx_(candidateIdx) {}

std::pair<bool, uint64_t> ElectionStub::elect(const uint64_t term,
                                              const int64_t lastLogIndex,
                                              const uint64_t lastLogTerm) {
  token::RequestVoteArgument request;

  request.set_candidateid(candidate_idx_);

  request.set_term(term);
  request.set_lastlogindex(lastLogIndex);
  request.set_lastlogterm(lastLogTerm);

  std::vector<token::RequestVoteResult> replies(raft_stubs_.size());
  std::vector<ClientContext> contexts(raft_stubs_.size());

  // Need At least half of other raft nodes to vote to be elected

  uint64_t votedNum = 0;
  uint64_t respondedNum = 0;
  std::mutex mux;
  std::condition_variable cv;

  for (uint64_t i = 0; i < raft_stubs_.size(); ++i) {
    if (i == candidate_idx_) {
      continue;
    }

    raft_stubs_[i]->async()->RequestVote(
        &contexts[i], &request, &replies[i],
        [&mux, &respondedNum, &replies, &votedNum, &cv,
         i](grpc::Status status) {
          std::unique_lock lock(mux);

          respondedNum++;

          if (status.ok() && replies[i].votegranted()) {
            votedNum++;
          }

          if (respondedNum + 1 == replies.size()) {
            cv.notify_all();
          }
        });
  }

  std::unique_lock lock(mux);

  cv.wait(lock, [&replies, &respondedNum]() {
    return respondedNum + 1 == replies.size();
  });

  uint64_t maxTerm = term;

  utility::logError("Vote count %u", votedNum);

  for (uint64_t i = 0; i < raft_stubs_.size(); ++i) {
    if (i == candidate_idx_) {
      continue;
    }

    maxTerm = std::max(maxTerm, replies[i].term());
  }

  return {(votedNum + 1) * 2 > replies.size(), maxTerm};
}

} // namespace stub
