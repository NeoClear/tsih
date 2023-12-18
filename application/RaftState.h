#pragma once

#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <random>
#include <string>
#include <string_view>

#include "absl/container/flat_hash_map.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "proto/api.grpc.pb.h"

#include "stub/AppendStub.h"
#include "stub/PingStub.h"

#include "utility/Deadliner.h"
#include "utility/Logger.h"
#include "utility/PingHistory.h"

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

constexpr std::string_view roleToSV(RaftRole role) {
  using namespace std::literals;
  switch (role) {
  case RaftRole::RAFT_LEADER:
    return "RAFT_LEADER"sv;
  case RaftRole::RAFT_FOLLOWER:
    return "RAFT_FOLLOWER"sv;
  case RaftRole::RAFT_CANDIDATE:
    return "RAFT_CANDIDATE"sv;
  }
}

/**
 * @brief Class that keeps the state of raft data structures
 *
 * The class is refered by RaftServiceImpl to modify the state on receiving
 * requests. It is better for code refactoring and modification
 */
class RaftState {
private:
  std::mutex mux_;

  uint64_t current_term_;

  std::optional<uint64_t> voted_for_;

  std::vector<std::pair<std::string, uint64_t>> log_;

  uint64_t commit_index_;
  uint64_t last_applied_;

  std::vector<uint64_t> next_index_;
  std::vector<uint64_t> match_index_;

  uint64_t candidate_idx_;

  /**
   * @brief Number of other raft nodes that granted vote for current node
   * OTHER NODES! Does not include itself
   */
  uint64_t granted_;

  uint64_t raft_size_;
  RaftRole role_;

  /**
   * @brief Called when candidate failed to get an result for election
   *
   * This deadliner is a tool to initiate another round of election
   */
  utility::Deadliner candidate_timeout_;

  /**
   * @brief Called when follower failed to get a ping from leader
   *
   * This deadliner is a tool to switch to switch to candidate from follower
   */
  utility::Deadliner follower_timeout_;

  /**
   * @brief Leader should periodically send ping messages to followers
   */
  utility::Deadliner leader_periodic_;

  stub::PingStub ping_stub_;
  stub::AppendStub append_stub_;

  /**
   * @brief Called when switched to leader
   */
  void switchToLeader();

  void switchToFollower();

  void switchToCandidate();

  // Append entries to log
  bool appendEntries(
      uint64_t prevLogIndex, uint64_t prevLogTerm,
      const google::protobuf::RepeatedPtrField<token::LogEntry>& appendLogs);

  void leaderElection();

  void incTerm();

  void updateTerm(const uint64_t newTerm);

  void followerTimeoutCallback();

  void leaderPeriodicCallback();

public:
  explicit RaftState(uint64_t raft_size, uint64_t candidate_idx)
      : current_term_(0), commit_index_(0), last_applied_(0),
        candidate_idx_(candidate_idx), granted_(0u), raft_size_(raft_size),
        role_(RaftRole::RAFT_CANDIDATE),
        candidate_timeout_(std::bind(&RaftState::leaderElection, this)),
        follower_timeout_(std::bind(&RaftState::followerTimeoutCallback, this)),
        leader_periodic_(std::bind(&RaftState::leaderPeriodicCallback, this)),
        ping_stub_(raft_size_, token::ServerType::MASTER, candidate_idx_),
        append_stub_(raft_size_) {
    candidate_timeout_.setRandomDeadline(300, 500);
  }

  std::string toString() const {
    return absl::StrFormat("Role: %s, term: %d", roleToSV(role_),
                           current_term_);
  }

  /**
   * @brief Handle append entries request to raft state
   */
  std::pair<uint64_t, bool> handleAppendEntries(
      uint64_t term, uint64_t leaderId, int64_t prevLogIndex,
      uint64_t prevLogTerm,
      const google::protobuf::RepeatedPtrField<token::LogEntry>& entries,
      uint64_t leaderCommit);

  /**
   * @brief Return values are send instructions
   *
   * @param term
   * @param candidateId
   * @param lastLogIndex
   * @param lastLogTerm
   */
  std::pair<uint64_t, bool> handleVoteRequest(uint64_t term,
                                              uint64_t candidateId,
                                              int64_t lastLogIndex,
                                              uint64_t lastLogTerm);

  /**
   * @brief Handle vote replies from other nodes
   *
   * @param term
   * @param voteGranted
   */
  void handleVoteReply(uint64_t term, bool voteGranted);

  void handlePingMsg(token::ServerType senderType, uint64_t senderIdx);

  void handleAddTask(std::string task);
};

} // namespace application
