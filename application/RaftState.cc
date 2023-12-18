#include "application/RaftState.h"

#include "stub/ElectionStub.h"
#include "utility/logger.h"

namespace application {

void RaftState::switchToLeader() {
  if (role_ == RaftRole::RAFT_LEADER) {
    return;
  }

  role_ = RaftRole::RAFT_LEADER;
  match_index_ = std::vector<uint64_t>(raft_size_, 0ull);
  utility::logError("Become leader on term %u", current_term_);
  leader_periodic_.setDeadline(100);
}

void RaftState::switchToFollower() {
  if (role_ == RaftRole::RAFT_FOLLOWER) {
    return;
  }

  role_ = RaftRole::RAFT_FOLLOWER;

  // Start the timeout
  follower_timeout_.setDeadline(500);
}

void RaftState::switchToCandidate() {
  if (role_ == RaftRole::RAFT_CANDIDATE) {
    return;
  }

  role_ = RaftRole::RAFT_CANDIDATE;

  candidate_timeout_.setRandomDeadline(300, 500);
}

bool RaftState::appendEntries(
    uint64_t prevLogIndex, uint64_t prevLogTerm,
    const google::protobuf::RepeatedPtrField<token::LogEntry> &appendLogs) {
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

void RaftState::leaderElection() {
  uint64_t raftSize = raft_size_;
  uint64_t candidateIdx = candidate_idx_;
  uint64_t currentTerm = current_term_;
  int64_t prevLogIdx;
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

    raftSize = raft_size_;
    candidateIdx = candidate_idx_;
    currentTerm = current_term_;

    prevLogIdx = log_.empty() ? -1 : log_.size() - 1;
    lastLogTerm = log_.empty() ? 0 : log_.back().second;
  }

  // Send voting information all other masters
  stub::ElectionStub election(raftSize, candidateIdx, currentTerm, prevLogIdx,
                              lastLogTerm);
  election.initiateElection();

  candidate_timeout_.setRandomDeadline(300, 500);
}

void RaftState::incTerm() {
  granted_ = 0;
  voted_for_.reset();
  current_term_++;
  utility::logInfo("Term incremented %u", current_term_);
}

void RaftState::updateTerm(const uint64_t newTerm) {
  assert(newTerm > current_term_);

  granted_ = 0;
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

    utility::logInfo("Leader heartbeating");
  }

  // Send to all followers a ping message
  token::ServerIdentity identity;
  identity.set_server_type(token::MASTER);
  identity.set_server_index(candidate_idx_);
  stub::PingStub stub(raft_size_, identity);
  stub.ping();

  leader_periodic_.setDeadline(100);
}

void RaftState::handleAppendEntries(
    uint64_t term, uint64_t leaderId, int64_t prevLogIndex,
    uint64_t prevLogTerm,
    const google::protobuf::RepeatedPtrField<token::LogEntry> &entries,
    uint64_t leaderCommit) {
  std::unique_lock lock(mux_);

  // Received outdated append request, reject the request
  if (term < current_term_) {
    // @TODO(Send reply)
    // return Status::OK;
    return;
  }

  if (term >= current_term_) {
    current_term_ = term;
    switchToFollower();
  }

  // At this stage, request term and local term are equal
  // No need to consider invalid requests
  switch (role_) {
  case RaftRole::RAFT_LEADER:
    throw std::runtime_error("No two leader within the same term");
    break;
  case RaftRole::RAFT_FOLLOWER: {
    bool success = appendEntries(prevLogIndex, prevLogTerm, entries);

    // TODO: send reply
    // reply->set_success(success);
    // reply->set_term(raft_state_.currentTerm);

    // Reset a timeout
    follower_timeout_.setRandomDeadline(500);
  }

  break;
  case RaftRole::RAFT_CANDIDATE:
    throw std::runtime_error("Cannot be RAFT_CANDIDATE at this stage");
    break;
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
    // stub::ElectionStub::replyTo(candidateId, current_term_, false);
    // return Status::OK;
    return {current_term_, false};
  } else if (term == current_term_) {
    // The voting request is the same as current term

    utility::logInfo("Staying at same term number %u", current_term_);

    switch (role_) {
    case RaftRole::RAFT_LEADER:
      // As a leader of the same term, reject the request
      // stub::ElectionStub::replyTo(candidateId, current_term_, false);
      return {current_term_, false};
      // break;
    case RaftRole::RAFT_FOLLOWER:
      // Already a follower following the current leader, reject the request
      // stub::ElectionStub::replyTo(candidateId, current_term_, false);
      return {current_term_, false};
      // break;
    case RaftRole::RAFT_CANDIDATE:
      // As a candidate looking for the same thing, approve it and set votedFor
      if (voted_for_ && (*voted_for_) != candidateId) {
        // Voted for someone else, reject it
        // stub::ElectionStub::replyTo(candidateId, current_term_, false);
        return {current_term_, false};
      } else {
        // Otherwise approve it and set voteFor
        voted_for_ = candidateId;
        // stub::ElectionStub::replyTo(candidateId, current_term_, true);
        return {current_term_, true};
      }

      break;
    default:
      throw std::runtime_error("Unrecognized RaftRole");
    }

  } else {
    utility::logInfo("Advancing to term number %u", term);

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
      // stub::ElectionStub::replyTo(candidateId, term, true);
      return {current_term_, true};

      // break;
    case RaftRole::RAFT_CANDIDATE:
      // Also switch to follower
      updateTerm(term);
      switchToFollower();
      voted_for_ = candidateId;

      // Voted
      // stub::ElectionStub::replyTo(candidateId, term, true);

      // break;
      return {current_term_, true};
    default:
      throw std::runtime_error("Unrecognized RaftRole");
    }
  }

  utility::logInfo("Raft state: %s", toString());
}

void RaftState::handleVoteReply(uint64_t term, bool voteGranted) {
  std::unique_lock lock(mux_);

  utility::logInfo("Received reply %u: %s", term,
                   (voteGranted ? "granted" : "rejected"));

  if (term < current_term_) {
    // Expired replies
    // Ignore
  } else if (term == current_term_) {
    if (voteGranted) {
      granted_++;
      utility::logInfo("Visited");
    }

    switch (role_) {
    case RaftRole::RAFT_LEADER:
      // Already a leader, do not care about replies
      break;
    case RaftRole::RAFT_FOLLOWER:
      // No idea why we have this branch
      break;
    case RaftRole::RAFT_CANDIDATE:
      if ((granted_ + 1) * 2 > raft_size_) {
        // Getting majority, switch to leader
        switchToLeader();
      }
      break;
    default:
      throw std::runtime_error("Invalid role");
      break;
    }
  } else {
    // Getting reply from future term number. Is it even possible?
    throw std::runtime_error("Getting future term number");
  }
}

void RaftState::handlePingMsg(token::ServerType senderType,
                              uint64_t senderIdx) {
  utility::logError("Reveived ping");
  if (senderType == token::ServerType::MASTER) {
    // Switch to follower
    switchToFollower();

    // Received a ping from leader
    follower_timeout_.setDeadline(500);
  }
}

} // namespace application