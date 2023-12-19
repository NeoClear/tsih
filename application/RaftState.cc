#include "application/RaftState.h"

#include "stub/ElectionStub.h"
#include "utility/Logger.h"

namespace application {

void RaftState::switchToLeader() {
  if (role_ == RaftRole::RAFT_LEADER) {
    return;
  }

  role_ = RaftRole::RAFT_LEADER;
  next_index_ = std::vector<uint64_t>(raft_size_, log_.size());
  match_index_ = std::vector<uint64_t>(raft_size_, 0ull);
  utility::logCrit("Become leader on term %u", current_term_);
  leader_periodic_.setDeadline(100);
}

void RaftState::switchToFollower() {
  if (role_ == RaftRole::RAFT_FOLLOWER) {
    return;
  }

  role_ = RaftRole::RAFT_FOLLOWER;

  utility::logInfo("switch to follower");

  // Start the timeout
  follower_timeout_.setDeadline(2000);
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
    uint64_t prevLogIndex, uint64_t prevLogTerm,
    const google::protobuf::RepeatedPtrField<token::LogEntry>& appendLogs) {
  assert(role_ == RaftRole::RAFT_FOLLOWER);

  bool match =
      prevLogIndex == 0ull ||
      (prevLogIndex < log_.size() && log_[prevLogIndex].second == prevLogTerm);

  if (!match) {
    return false;
  }

  // On match, commit log entries
  for (uint64_t idx = 0; idx < appendLogs.size(); ++idx) {
    uint64_t logIdx = idx + prevLogIndex + 1;

    if (logIdx < log_.size()) {
      log_[logIdx].first = appendLogs[idx].content();
      log_[logIdx].second = appendLogs[idx].term();
    } else {
      log_.emplace_back(appendLogs[idx].content(), appendLogs[idx].term());
    }
  }

  return true;
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
  {
    std::unique_lock lock(mux_);

    if (role_ != RaftRole::RAFT_LEADER) {
      return;
    }
  }

  // Send to all followers a ping message
  // Sending ping messages
  utility::logInfo("Pinging others");
  ping_stub_.ping();

  leader_periodic_.setDeadline(100);
}

RaftState::RaftState(uint64_t raftSize, uint64_t candidateIdx)
    : raft_size_(raftSize), candidate_idx_(candidateIdx),
      role_(RaftRole::RAFT_CANDIDATE), current_term_(0), commit_index_(0),
      last_applied_(0),
      candidate_timeout_(std::bind(&RaftState::candidateTimeoutCallback, this)),
      follower_timeout_(std::bind(&RaftState::followerTimeoutCallback, this)),
      leader_periodic_(std::bind(&RaftState::leaderPeriodicCallback, this)),
      ping_stub_(raft_size_, token::ServerType::MASTER, candidate_idx_),
      append_stub_(raft_size_), election_stub_(raft_size_, candidate_idx_) {
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

    // Reset a timeout
    follower_timeout_.setRandomDeadline(500);

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
  } else if (term == current_term_) {
    // The voting request is the same as current term

    utility::logInfo("Staying at same term number %u", current_term_);

    switch (role_) {
    case RaftRole::RAFT_LEADER:
      // As a leader of the same term, reject the request
      return {current_term_, false};
    case RaftRole::RAFT_FOLLOWER:
      // Already a follower following the current leader, reject the request
      return {current_term_, false};
    case RaftRole::RAFT_CANDIDATE:
      // As a candidate looking for the same thing, approve it and set votedFor
      if (voted_for_ && (*voted_for_) != candidateId) {
        // Voted for someone else, reject it
        return {current_term_, false};
      } else {
        /**
         * Only vote for other candidates with longer log entries
         */
        if (lastLogIndex + 1 > log_.size()) {
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

  } else {
    // utility::logInfo("Advancing to term number %u", term);
    // utility::logInfo("Current role %s", roleToSV(role_));

    // Larger term number
    switch (role_) {
    case RaftRole::RAFT_LEADER:
      // Become a follower
      updateTerm(term);
      switchToFollower();
      voted_for_ = candidateId;

      // Voted
      return {current_term_, true};
    case RaftRole::RAFT_FOLLOWER:
      // Already a follower, update the term number and grant it if not granted
      updateTerm(term);
      voted_for_ = candidateId;

      // Voted
      return {current_term_, true};
    case RaftRole::RAFT_CANDIDATE:
      // Also switch to follower
      updateTerm(term);
      switchToFollower();
      voted_for_ = candidateId;

      // Voted
      return {current_term_, true};
    default:
      throw std::runtime_error("Unrecognized RaftRole");
    }
  }
}

void RaftState::handlePing(token::ServerType senderType, uint64_t senderIdx) {
  utility::logError("Received ping from leader");

  if (senderType == token::ServerType::MASTER) {
    // Switch to follower
    std::unique_lock lock(mux_);

    switchToFollower();

    // Received a ping from leader
    follower_timeout_.setDeadline(500);
  }
}

void RaftState::handleAddTask(std::string task) {
  std::unique_lock lock(mux_);

  // Discard the request if node is not a leader
  if (role_ != RaftRole::RAFT_LEADER) {
    return;
  }

  utility::logInfo("Handling %s", task);

  // Replicate the log to other servers

  // For current thread, simply append to log entry, and a background thread
  // would handle that
  log_.emplace_back(task, current_term_);
}

} // namespace application
