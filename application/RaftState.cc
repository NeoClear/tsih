#include "application/RaftState.h"

#include "stub/ElectionStub.h"
#include "utility/Logger.h"

namespace application {

std::vector<std::unique_ptr<RaftService::Stub>>
RaftState::initRaftStubs(uint64_t raftSize, uint64_t candidateIdx) {
  std::vector<std::unique_ptr<RaftService::Stub>> stubs;

  for (uint64_t port = MASTER_BASE_PORT; port < MASTER_BASE_PORT + raftSize;
       ++port) {
    if (port == MASTER_BASE_PORT + candidateIdx) {
      stubs.emplace_back(nullptr);
    } else {

      std::shared_ptr<Channel> channel =
          grpc::CreateChannel(absl::StrFormat("0.0.0.0:%u", port),
                              grpc::InsecureChannelCredentials());
      stubs.emplace_back(RaftService::NewStub(channel));
    }
  }

  return stubs;
}

void RaftState::switchToLeader() {
  if (role_ == RaftRole::RAFT_LEADER) {
    return;
  }

  role_ = RaftRole::RAFT_LEADER;

  for (uint64_t i = 0; i < raft_size_; ++i) {
    // std::unique_lock lock(master_mutex_[i]);

    next_index_[i] = log_.size();
    match_index_[i] = 0;
  }

  // next_index_ = std::vector<uint64_t>(raft_size_, log_.size());
  // match_index_ = std::vector<uint64_t>(raft_size_, 0ull);
  utility::logCrit("Become leader on term %u", current_term_);
  leader_periodic_.setDeadline(100);
  task_execution_periodic_.setDeadline(1000);
}

void RaftState::switchToFollower() {
  if (role_ == RaftRole::RAFT_FOLLOWER) {
    return;
  }

  role_ = RaftRole::RAFT_FOLLOWER;

  utility::logInfo("switch to follower");

  // Start the timeout
  // The timer is only for
  follower_timeout_.setRandomDeadline(200, 1000);
}

void RaftState::switchToCandidate() {
  if (role_ == RaftRole::RAFT_CANDIDATE) {
    return;
  }

  role_ = RaftRole::RAFT_CANDIDATE;

  utility::logInfo("Switch to candidate");
  candidate_timeout_.setRandomDeadline(200, 2000);
}

bool RaftState::appendEntries(
    int64_t prevLogIndex, uint64_t prevLogTerm,
    const google::protobuf::RepeatedPtrField<token::LogEntry>& appendLogs) {
  assert(role_ == RaftRole::RAFT_FOLLOWER);

  bool match =
      prevLogIndex == -1ull ||
      (prevLogIndex < log_.size() && log_[prevLogIndex].second == prevLogTerm);

  if (!match) {
    return false;
  }

  // On match, commit log entries
  for (uint64_t idx = 0; idx < appendLogs.size(); ++idx) {
    uint64_t logIdx = idx + prevLogIndex + 1;

    if (logIdx < log_.size()) {
      log_[logIdx].first = RaftLog::buildRaftLog(appendLogs[idx].content());
      log_[logIdx].second = appendLogs[idx].term();
    } else {
      log_.emplace_back(RaftLog::buildRaftLog(appendLogs[idx].content()),
                        appendLogs[idx].term());
    }
  }

  return true;
}

void RaftState::commitUpTo(uint64_t endIndex) {
  // Must be locked
  assert(commit_index_ <= endIndex);

  uint64_t applyEnd = std::min<uint64_t>(endIndex, log_.size());

  for (uint64_t logIndex = last_applied_; logIndex < applyEnd; ++logIndex) {
    log_[logIndex].first->applyLog(task_schedule_, logIndex);
  }

  if (last_applied_ != applyEnd) {
    utility::logInfo("Commited up to %u", applyEnd);
  }

  last_applied_ = applyEnd;
  commit_index_ = endIndex;
}

void RaftState::candidateTimeoutCallback() {
  uint64_t currentTerm;
  int64_t lastLogIdx;
  uint64_t lastLogTerm;

  {
    // leader election potentially modifies raft state, thus needs to be
    // protected by mutex
    std::unique_lock lock(mux_);

    // Not a candidate, meaning things has come to an end, no need to perform
    // works
    if (role_ != RaftRole::RAFT_CANDIDATE) {
      return;
    }

    // Move to new term
    incTerm();

    // Vote for self
    voted_for_ = candidate_idx_;

    currentTerm = current_term_;
    lastLogIdx = log_.empty() ? -1 : log_.size() - 1;
    lastLogTerm = log_.empty() ? 0 : log_.back().second;
  }

  // Send voting information all other masters
  auto [elected, maxTerm] =
      election_stub_.elect(currentTerm, lastLogIdx, lastLogTerm);
  // elected should be computed within timely manner, assuming it takes short
  // amount of time

  if (elected) {
    std::unique_lock lock(mux_);

    if (current_term_ == currentTerm && maxTerm == currentTerm) {
      switchToLeader();

      return;
    }

    if (maxTerm > current_term_) {
      updateTerm(maxTerm);
    }
  }

  // Not elected, go for another round of election after some random amount of
  // time
  candidate_timeout_.setRandomDeadline(200, 2000);
}

void RaftState::syncWorker() {
  /**
   * @brief Must be called with lock on
   */
  const auto clearRequestQueue = [this]() {
    while (!request_queue_.empty()) {
      request_queue_.front().second.set_value({false, 0});
      request_queue_.pop();
    }
  };

  // An infinite loop
  for (;;) {
    std::unique_lock lock(mux_);
    on_process_request_.wait(lock,
                             [this]() { return !request_queue_.empty(); });

    if (role_ != RaftRole::RAFT_LEADER) {
      // Not a leader but have pending requests, reject all of them
      clearRequestQueue();

      continue;
    }

    std::pair<std::unique_ptr<RaftLog>, std::promise<std::pair<bool, uint64_t>>>
        currentRequest = std::move(request_queue_.front());

    request_queue_.pop();

    // Current master node is still a leader, append to log
    log_.emplace_back(std::move(currentRequest.first), current_term_);

    uint64_t logSize = log_.size();

    std::condition_variable onAppendEntriesFinished;

    // Loop until one of the following:
    // 1. Unable to commit
    // 2. Successfully commited
    for (;;) {
      /**
       * @brief Prepare rpc docs
       */
      std::vector<ClientContext> contexts(raft_size_);
      std::vector<AppendEntriesArgument> requests(raft_size_);
      std::vector<AppendEntriesResult> replies(raft_size_);
      std::vector<bool> rpcStatus(raft_size_);
      std::vector<bool> requireRPC(raft_size_, true);
      requireRPC[candidate_idx_] = false;

      // Prepare requests
      for (uint64_t i = 0; i < raft_size_; ++i) {
        if (!requireRPC[i]) {
          continue;
        }

        requests[i].set_term(current_term_);
        requests[i].set_leaderid(candidate_idx_);
        requests[i].set_prevlogindex(static_cast<int64_t>(next_index_[i]) -
                                     1ll);
        requests[i].set_prevlogterm(
            next_index_[i] == 0 ? 0 : log_[next_index_[i] - 1].second);
        requests[i].set_leadercommit(log_.size() - 1);

        // Goes all the way to the end
        for (uint64_t copyIdx = next_index_[i]; copyIdx < log_.size();
             ++copyIdx) {
          token::LogEntry* entryIt = requests[i].add_entries();
          // entryIt->set_content(log_[copyIdx].first);
          *entryIt->mutable_content() = log_[copyIdx].first->buildLogEntry();

          entryIt->set_term(log_[copyIdx].second);
        }
      }

      bool rpcFinished = false;

      /**
       * @brief Wait for rpc completion
       */
      std::thread rpcWaiter([this, &requireRPC, &contexts, &requests, &replies,
                             &rpcStatus, &rpcFinished,
                             &onAppendEntriesFinished]() {
        append_stub_.sendAppendEntriesRequest(requireRPC, contexts, requests,
                                              replies, rpcStatus);
        std::unique_lock lock(mux_);
        rpcFinished = true;
        onAppendEntriesFinished.notify_all();
      });

      onAppendEntriesFinished.wait(lock,
                                   [&rpcFinished]() { return rpcFinished; });

      rpcWaiter.join();

      uint64_t failedNum = 0;
      bool outdated = false;

      /**
       * @brief Check rpc results
       *
       * There are multiple cases:
       * 1. Current master is no longer a leader.
       *    In this case, clear all requests
       * 2. Exist a reply saying you are out of date
       *    for this case, also clear all requests
       * 3. I am the leader, all replies agree with the term number, and:
       *    3.1. Majority accepts
       *         for this case, break the loop and reply client with a success
       *    3.2. No majority accepts
       *         update nextIdx and try again
       */

      // No longer leader, clear queue and wait for the next phase
      if (role_ != RaftRole::RAFT_LEADER) {
        currentRequest.second.set_value({false, 0});
        clearRequestQueue();
        break;
      }

      for (uint64_t i = 0; i < raft_size_; ++i) {
        if (!requireRPC[i]) {
          continue;
        }

        if (!rpcStatus[i]) {
          failedNum++;
          continue;
        }

        if (replies[i].term() > current_term_) {
          // Term outdated, no longer leader
          outdated = true;
          break;
        }

        if (replies[i].success()) {
          requireRPC[i] = false;

          // It is actually size, not index
          match_index_[i] = logSize; // logSize nubmer of logs are matched
          next_index_[i] = logSize;  // The next index would be logSize
        } else {
          // Not successful, continue
          uint64_t targetLogSize = replies[i].logsize();

          assert(next_index_[i] > 0);

          next_index_[i] = std::min(next_index_[i] - 1, targetLogSize);
        }
      }

      if (outdated) {
        currentRequest.second.set_value({false, 0});
        clearRequestQueue();
        switchToFollower();
        break;
      }

      if (failedNum * 2 >= raft_size_) {
        // Majority failed, cannot process the request
        currentRequest.second.set_value({false, 0});
        break;
      }

      // If majority have accepted the request, mark request as fullfilled
      if (std::count(requireRPC.cbegin(), requireRPC.cend(), false) * 2 >
          raft_size_) {
        currentRequest.second.set_value({true, logSize - 1});
        // Do not forget to update the commit log
        commitUpTo(logSize);
        break;
      } else {
        // Otherwise, retry algorithm with more logs
      }
    }
  }
}

void RaftState::incTerm() {
  voted_for_.reset();
  current_term_++;
  utility::logInfo("Term incremented %u", current_term_);
}

void RaftState::updateTerm(const uint64_t newTerm) {
  assert(newTerm > current_term_);

  voted_for_.reset();
  current_term_ = newTerm;
}

void RaftState::followerTimeoutCallback() {
  std::unique_lock lock(mux_);

  assert(role_ == RaftRole::RAFT_FOLLOWER);

  switchToCandidate();
}

void RaftState::leaderPeriodicCallback() {
  uint64_t logSize;

  {
    std::unique_lock lock(mux_);

    if (role_ != RaftRole::RAFT_LEADER) {
      return;
    }

    logSize = log_.size();
  }

  // Send to all followers a ping message
  // Sending ping messages

  // Send empty AppendEntries rpc to followers
  std::vector<ClientContext> contexts(raft_size_);
  std::vector<AppendEntriesArgument> requests(raft_size_);
  std::vector<AppendEntriesResult> replies(raft_size_);
  std::vector<bool> rpcStatus(raft_size_);
  std::vector<bool> requireRPC(raft_size_, true);
  requireRPC[candidate_idx_] = false;

  // Prepare requests
  for (uint64_t i = 0; i < raft_size_; ++i) {
    if (!requireRPC[i]) {
      continue;
    }

    requests[i].set_term(current_term_);
    requests[i].set_leaderid(candidate_idx_);
    requests[i].set_prevlogindex(static_cast<int64_t>(logSize) - 1ll);
    requests[i].set_prevlogterm(logSize == 0 ? 0 : log_[logSize - 1].second);
    requests[i].set_leadercommit(commit_index_);
  }

  append_stub_.sendAppendEntriesRequest(requireRPC, contexts, requests, replies,
                                        rpcStatus);

  leader_periodic_.setDeadline(100);
}

void RaftState::taskExecutionPeriodicCallback() {
  {
    std::unique_lock lock(mux_);

    if (role_ != RaftRole::RAFT_LEADER) {
      return;
    }

    absl::flat_hash_map<uint64_t, uint64_t> taskAssignment =
        task_schedule_.getTaskAssignment();

    for (const auto [taskId, workerIndex] : taskAssignment) {
      token::TaskActionEntry actionEntry;
      actionEntry.mutable_assigntaskentry()->set_taskid(taskId);
      actionEntry.mutable_assigntaskentry()->set_workerindex(workerIndex);

      // Add to queue and notify sync thread to continue processing works
      request_queue_.emplace(RaftLog::buildRaftLog(actionEntry),
                             std::promise<std::pair<bool, uint64_t>>());
    }

    on_process_request_.notify_all();
  }

  task_execution_periodic_.setDeadline(1000);
}

RaftState::RaftState(uint64_t raftSize, uint64_t candidateIdx)
    : raft_size_(raftSize), candidate_idx_(candidateIdx),
      role_(RaftRole::RAFT_CANDIDATE), current_term_(0), commit_index_(0),
      last_applied_(0), next_index_(raft_size_), match_index_(raft_size_),
      raft_stubs_(initRaftStubs(raft_size_, candidate_idx_)),
      candidate_timeout_(std::bind(&RaftState::candidateTimeoutCallback, this)),
      follower_timeout_(std::bind(&RaftState::followerTimeoutCallback, this)),
      leader_periodic_(std::bind(&RaftState::leaderPeriodicCallback, this)),
      task_execution_periodic_(
          std::bind(&RaftState::taskExecutionPeriodicCallback, this)),
      append_stub_(raft_stubs_), election_stub_(raft_stubs_, candidate_idx_),
      sync_thread_(std::bind(&RaftState::syncWorker, this)) {

  candidate_timeout_.setRandomDeadline(200, 2000);
}

std::pair<uint64_t, bool> RaftState::handleAppendEntries(
    uint64_t term, uint64_t leaderId, int64_t prevLogIndex,
    uint64_t prevLogTerm,
    const google::protobuf::RepeatedPtrField<token::LogEntry>& entries,
    uint64_t leaderCommit) {
  std::unique_lock lock(mux_);

  // Received outdated append request, reject the request
  if (term < current_term_) {
    return {current_term_, false};
  }

  if (term > current_term_) {
    updateTerm(term);
  }

  switchToFollower();

  // At this stage, request term and local term are equal
  // No need to consider invalid requests
  switch (role_) {
  case RaftRole::RAFT_LEADER:
    throw std::runtime_error("No two leader within the same term");
  case RaftRole::RAFT_FOLLOWER: {
    bool success = appendEntries(prevLogIndex, prevLogTerm, entries);

    // Update commit_index_ and apply changes
    if (success) {
      commitUpTo(leaderCommit);
    }

    // Reset a timeout
    follower_timeout_.setDeadline(500);

    return {current_term_, success};
  }

  break;
  case RaftRole::RAFT_CANDIDATE:
    throw std::runtime_error("Cannot be RAFT_CANDIDATE at this stage");
  default:
    throw std::runtime_error("Unrecognized RaftRole");
  }
}

std::pair<uint64_t, bool> RaftState::handleVoteRequest(uint64_t term,
                                                       uint64_t candidateId,
                                                       int64_t lastLogIndex,
                                                       uint64_t lastLogTerm) {
  std::unique_lock lock(mux_);

  if (term < current_term_) {
    // Would not vote for outdated term number
    // Send reject message back
    return {current_term_, false};
  } else {
    // The voting request is the same as current term

    // utility::logInfo("Staying at same term number %u", current_term_);
    if (term > current_term_) {
      switchToFollower();
      updateTerm(term);
    }

    switch (role_) {
    case RaftRole::RAFT_LEADER:
      // As a leader of the same term, reject the request
      return {current_term_, false};
    case RaftRole::RAFT_FOLLOWER:
      // // Already a follower following the current leader, reject the
      // request return {current_term_, false};
    case RaftRole::RAFT_CANDIDATE:
      // As a candidate looking for the same thing, approve it and set
      // votedFor
      if (voted_for_ && (*voted_for_) != candidateId) {
        // Voted for someone else, reject it
        return {current_term_, false};
      } else {
        /**
         * Only vote for other candidates with longer log entries
         */
        if (lastLogIndex + 1 >= log_.size()) {
          voted_for_ = candidateId;
          return {current_term_, true};
        } else {
          return {current_term_, false};
        }
      }

      break;
    default:
      throw std::runtime_error("Unrecognized RaftRole");
    }
  }
}

void RaftState::handlePing(uint64_t workerIndex,
                           std::vector<uint64_t> runningTaskIds) {
  // utility::logError("Received ping from leader");

  task_schedule_.workerPing(workerIndex, runningTaskIds);
}

std::future<std::pair<bool, uint64_t>>
RaftState::handleTaskSubmission(std::string task) {
  std::unique_lock lock(mux_);

  std::promise<std::pair<bool, uint64_t>> promise;
  std::future<std::pair<bool, uint64_t>> future = promise.get_future();

  // Discard the request if node is not a leader
  if (role_ != RaftRole::RAFT_LEADER) {
    promise.set_value({false, 0});
    return future;
  }

  token::TaskActionEntry actionEntry;
  actionEntry.mutable_submittaskentry()->set_value(task);

  // Add to queue and notify sync thread to continue processing works
  request_queue_.emplace(RaftLog::buildRaftLog(actionEntry),
                         std::move(promise));
  on_process_request_.notify_all();

  /**
   * @todo Wake up background thread to perform sync
   */

  return future;
}

token::TaskStatus RaftState::handleTaskQuery(uint64_t taskId) {
  return task_schedule_.queryTaskStatus(taskId);
}

uint64_t RaftState::handleServiceQuery(token::TaskStatus status) {
  switch (status) {
  case token::TaskStatus::PENDING:
    return task_schedule_.getPendingTasks();
  case token::TaskStatus::RUNNING:
    return task_schedule_.getRunningTasks();
  case token::TaskStatus::FAILED:
    return task_schedule_.getFinishedTasks();
  case token::TaskStatus::SUCCEEDED:
    return task_schedule_.getFinishedTasks();
  case token::TaskStatus::UNKNOWN:
    return task_schedule_.getWorkerCount();
  default:
    return UINT64_MAX;
  }
}

/**
 * @brief In real life this destructor would never get called, since it is a
 * long living object
 */
RaftState::~RaftState() {
  if (sync_thread_.joinable()) {
    sync_thread_.join();
  }
}

} // namespace application
