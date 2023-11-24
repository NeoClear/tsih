#pragma once

#include <iostream>
#include <memory>
#include <queue>
#include <random>
#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "proto/api.grpc.pb.h"

#include "stub/pinger.h"
#include "utility/deadliner.h"
#include "utility/ping_history.h"

#include <google/protobuf/util/message_differencer.h>

#include "config.h"

using api::PingTracker;
using api::RaftService;

using token::AppendEntriesArgument;
using token::AppendEntriesResult;
using token::PingMessage;
using token::RequestVoteArgument;
using token::RequestVoteResult;
using token::ServerIdentity;

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

namespace application {

enum class RaftRole {
  RAFT_LEADER = 0,
  RAFT_FOLLOWER = 1,
  RAFT_CANDIDATE = 2,
};

/**
 * @brief Class that keeps the state of raft data structures
 *
 * The class is refered by RaftServiceImpl to modify the state on receiving
 * requests. It is better for code refactoring and modification
 */
struct RaftState {
  uint64_t currentTerm;

  std::optional<uint64_t> votedFor;

  std::vector<std::pair<std::string, uint64_t>> log;

  uint64_t commitIndex;
  uint64_t lastApplied;

  std::vector<uint64_t> nextIndex;
  std::vector<uint64_t> matchIndex;

  uint64_t candidate_idx_;
  uint64_t raft_size_;
  RaftRole role_;

  explicit RaftState(uint64_t raft_size, uint64_t candidate_idx)
      : currentTerm(0), commitIndex(0), lastApplied(0),
        candidate_idx_(candidate_idx), raft_size_(raft_size),
        role_(RaftRole::RAFT_CANDIDATE) {}

  /**
   * @brief Called when switched to leader
   */
  void switchToLeader() {
    if (role_ == RaftRole::RAFT_LEADER) {
      return;
    }

    // nextIndex.swap(std::vector<uint64_t>(raft_size_, 0));
    matchIndex = std::vector<uint64_t>(raft_size_, 0ull);
  }

  void switchToFollower() {
    if (role_ == RaftRole::RAFT_FOLLOWER) {
      return;
    }

    // Start the timeout
  }

  void switchToCandidate() {}

  // Append entries to log
  bool appendEntries(
      uint64_t prevLogIndex, uint64_t prevLogTerm,
      const google::protobuf::RepeatedPtrField<token::LogEntry> &appendLogs);

  void leaderElection();

  void incTerm();

  void updateTerm(const uint64_t newTerm);

  std::string toString() const {
    return absl::StrFormat("Role: %d, term: %d", role_, currentTerm);
  }
};

class RaftServiceImpl : public api::RaftService::Service {
public:
  explicit RaftServiceImpl(uint64_t raft_size, uint64_t candidate_idx)
      : raft_state_(raft_size, candidate_idx),
        leaderTimeout(std::bind(&RaftState::leaderElection, raft_state_)) {

    std::random_device randDev;
    std::mt19937 rng(randDev());
    std::uniform_int_distribution<std::mt19937::result_type> distribution(150,
                                                                          300);
    leaderTimeout.setDeadline(distribution(rng));
  }

  Status AppendEntriesRequest(ServerContext *context,
                              const AppendEntriesArgument *request,
                              google::protobuf::Empty *reply);
  Status AppendEntriesReply(ServerContext *context,
                            const AppendEntriesResult *request,
                            google::protobuf::Empty *reply);
  Status RequestVoteRequest(ServerContext *context,
                            const RequestVoteArgument *request,
                            google::protobuf::Empty *reply);
  Status RequestVoteReply(ServerContext *context,
                          const RequestVoteResult *request,
                          google::protobuf::Empty *reply);

private:
  RaftState raft_state_;
  utility::Deadliner leaderTimeout;
};

} // namespace application
