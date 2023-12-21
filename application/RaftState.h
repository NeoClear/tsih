#pragma once

#include <future>
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
#include "stub/ElectionStub.h"
#include "stub/PingStub.h"

#include "application/TaskSchedule.h"

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
 *
 * @todo For now, the state of RaftState object is protected by a lock. This
 * method is pretty coarse-grained, and we are aiming to change it in the future
 */
class RaftState {
private:
  std::mutex mux_;

  /**
   * @brief Constant throughout the lifetime
   */
  const uint64_t raft_size_;
  const uint64_t candidate_idx_;

  RaftRole role_;

  /**
   * @brief It would be better to refer Raft payer
   */
  uint64_t current_term_;
  std::optional<uint64_t> voted_for_;
  std::vector<std::pair<std::string, uint64_t>> log_;
  uint64_t commit_index_;
  uint64_t last_applied_;
  std::vector<uint64_t> next_index_;
  std::vector<uint64_t> match_index_;

  /**
   * @brief Raft stubs
   */
  const std::vector<std::unique_ptr<RaftService::Stub>> raft_stubs_;

  static std::vector<std::unique_ptr<RaftService::Stub>>
  initRaftStubs(uint64_t raftSize, uint64_t candidateIdx);

  /**
   * @brief The queue holding requests
   *
   * Each element is a pair of request content and promise object for task
   * completion
   */
  std::queue<std::pair<std::string, std::promise<std::pair<bool, uint64_t>>>>
      request_queue_;
  // Paired with request_queue_ to notify thread when is time to continue
  // processing
  // Paired with RaftState global lock mux_ for proper synchronization
  // This CV is notified on either: 1. more requests coming. 2. Promoted to
  // leader
  std::condition_variable on_process_request_;

  application::TaskSchedule task_schedule_;

  /**
   * @brief Following are timers used to run specific tasks on timeout
   */
  utility::Deadliner candidate_timeout_;
  utility::Deadliner follower_timeout_;
  utility::Deadliner leader_periodic_;

  void candidateTimeoutCallback();
  void followerTimeoutCallback();
  void leaderPeriodicCallback();

  /**
   * @brief Following stubs are used to communicate with other nodes
   * Communications are multiplexed but synchronous, thus must be called outside
   * of lock protection, otherwise could result in deadlock
   */
  stub::PingStub ping_stub_;
  stub::AppendStub append_stub_;
  stub::ElectionStub election_stub_;

  /**
   * @brief Following functions are called to switch role
   * Must be called under lock protection
   */
  void switchToLeader();
  void switchToFollower();
  void switchToCandidate();

  // Append entries to log
  bool appendEntries(
      int64_t prevLogIndex, uint64_t prevLogTerm,
      const google::protobuf::RepeatedPtrField<token::LogEntry>& appendLogs);

  /**
   * @brief The background thread used to sync logs
   */
  void syncWorker();
  std::thread sync_thread_;

  /**
   * @brief Term update, must be protected by lock
   */
  void incTerm();
  void updateTerm(const uint64_t newTerm);

public:
  explicit RaftState(uint64_t raftSize, uint64_t candidateIdx);

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
   * @brief Corresponds with requestVote in Raft Paper
   * Do not handle response to avoid having both lock and communication
   */
  std::pair<uint64_t, bool> handleVoteRequest(uint64_t term,
                                              uint64_t candidateId,
                                              int64_t lastLogIndex,
                                              uint64_t lastLogTerm);

  void handlePing(token::ServerType senderType, uint64_t senderIdx);

  /**
   * @brief Return {true, job_id} on successful submission, {false, _} otherwise
   */
  std::future<std::pair<bool, uint64_t>> handleTaskSubmission(std::string task);

  ~RaftState();
};

} // namespace application
