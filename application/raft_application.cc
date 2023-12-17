#include "application/raft_application.h"
#include "stub/election.h"
#include "utility/logger.h"

namespace application {

bool RaftState::appendEntries(
    uint64_t prevLogIndex, uint64_t prevLogTerm,
    const google::protobuf::RepeatedPtrField<token::LogEntry> &appendLogs) {
  assert(role_ == RaftRole::RAFT_FOLLOWER);

  bool match =
      prevLogIndex == 0ull ||
      (prevLogIndex < log.size() && log[prevLogIndex].second == prevLogTerm);

  if (!match) {
    return false;
  }

  // On match, commit log entries
  for (uint64_t idx = 0; idx < appendLogs.size(); ++idx) {
    uint64_t logIdx = idx + prevLogIndex + 1;

    if (logIdx < log.size()) {
      log[logIdx].first = appendLogs[idx].content();
      log[logIdx].second = appendLogs[idx].term();
    } else {
      log.emplace_back(appendLogs[idx].content(), appendLogs[idx].term());
    }
  }

  return true;
}

void RaftState::leaderElection() {
  switchToCandidate();

  // Move to new term
  incTerm();

  int64_t prevLogIdx = log.empty() ? -1 : log.size() - 1;
  uint64_t lastLogTerm = log.empty() ? 0 : log.back().second;

  // Send voting information all other masters
  stub::ElectionStub election(raft_size_, candidate_idx_, currentTerm,
                              prevLogIdx, lastLogTerm);
  election.initiateElection();
}

void RaftState::incTerm() {
  granted = 0;
  votedFor.reset();
  currentTerm++;
  utility::logInfo("Term incremented %p", this);
}

void RaftState::updateTerm(const uint64_t newTerm) {
  assert(newTerm > currentTerm);

  granted = 0;
  votedFor.reset();
  currentTerm = newTerm;
}

Status
RaftServiceImpl::AppendEntriesRequest(ServerContext *context,
                                      const AppendEntriesArgument *request,
                                      google::protobuf::Empty *reply) {
  // Received outdated append request, reject the request
  if (request->term() < raft_state_.currentTerm) {
    // @TODO(Send reply)
    return Status::OK;
  }

  if (request->term() >= raft_state_.currentTerm) {
    raft_state_.currentTerm = request->term();
    raft_state_.switchToFollower();
  }

  // At this stage, request term and local term are equal
  // No need to consider invalid requests
  switch (raft_state_.role_) {
  case RaftRole::RAFT_LEADER:
    throw std::runtime_error("No two leader within the same term");
    break;
  case RaftRole::RAFT_FOLLOWER: {
    bool success = raft_state_.appendEntries(
        request->prevlogindex(), request->prevlogterm(), request->entries());

    // TODO: send reply
    // reply->set_success(success);
    // reply->set_term(raft_state_.currentTerm);

    // Reset a timeout
    // Something between 150 to 300 ms
    std::random_device randDev;
    std::mt19937 rng(randDev());
    std::uniform_int_distribution<std::mt19937::result_type> distribution(150,
                                                                          300);
    leaderTimeout.setDeadline(distribution(rng));
  }

  break;
  case RaftRole::RAFT_CANDIDATE:
    throw std::runtime_error("Cannot be RAFT_CANDIDATE at this stage");
    break;
  default:
    throw std::runtime_error("Unrecognized RaftRole");
  }

  return Status::OK;
}

Status RaftServiceImpl::AppendEntriesReply(ServerContext *context,
                                           const AppendEntriesResult *request,
                                           google::protobuf::Empty *reply) {
  return Status::OK;
}

/**
 * @brief Request vote to the current node
 *
 * @param context Misc
 * @param request Request containing vote request information
 * @param reply Vote result
 * @return Status
 */
Status RaftServiceImpl::RequestVoteRequest(ServerContext *context,
                                           const RequestVoteArgument *request,
                                           google::protobuf::Empty *reply) {
  if (request->term() < raft_state_.currentTerm) {
    // Would not vote for outdated term number
    // Send reject message back
    stub::ElectionStub::replyTo(request->candidateid(), raft_state_.currentTerm,
                                false);
    return Status::OK;
  } else if (request->term() == raft_state_.currentTerm) {
    // The voting request is the same as current term

    utility::logInfo("Staying at same term number %u", raft_state_.currentTerm);

    switch (raft_state_.role_) {
    case RaftRole::RAFT_LEADER:
      // As a leader of the same term, reject the request
      stub::ElectionStub::replyTo(request->candidateid(),
                                  raft_state_.currentTerm, false);
      break;
    case RaftRole::RAFT_FOLLOWER:
      // Already a follower following the current leader, reject the request
      stub::ElectionStub::replyTo(request->candidateid(),
                                  raft_state_.currentTerm, false);
      break;
    case RaftRole::RAFT_CANDIDATE:
      // As a candidate looking for the same thing, approve it and set votedFor
      if (raft_state_.votedFor &&
          (*raft_state_.votedFor) != request->candidateid()) {
        // Voted for someone else, reject it
        stub::ElectionStub::replyTo(request->candidateid(),
                                    raft_state_.currentTerm, false);
      } else {
        // Otherwise approve it and set voteFor
        raft_state_.votedFor = request->candidateid();
        stub::ElectionStub::replyTo(request->candidateid(),
                                    raft_state_.currentTerm, true);
      }

      break;
    default:
      throw std::runtime_error("Unrecognized RaftRole");
    }

  } else {
    utility::logInfo("Advancing to term number %u", request->term());

    // Larger term number
    switch (raft_state_.role_) {
    case RaftRole::RAFT_LEADER:
      // Become a follower
      raft_state_.updateTerm(request->term());
      raft_state_.switchToFollower();
      raft_state_.votedFor = request->candidateid();

      // Voted
      stub::ElectionStub::replyTo(request->candidateid(), request->term(),
                                  true);

      break;
    case RaftRole::RAFT_FOLLOWER:
      // Already a follower, update the term number and grant it if not granted
      raft_state_.updateTerm(request->term());
      raft_state_.votedFor = request->candidateid();

      // Voted
      stub::ElectionStub::replyTo(request->candidateid(), request->term(),
                                  true);

      break;
    case RaftRole::RAFT_CANDIDATE:
      // Also switch to follower
      raft_state_.updateTerm(request->term());
      raft_state_.switchToFollower();
      raft_state_.votedFor = request->candidateid();

      // Voted
      stub::ElectionStub::replyTo(request->candidateid(), request->term(),
                                  true);

      break;
    default:
      throw std::runtime_error("Unrecognized RaftRole");
    }
  }

  utility::logInfo("Raft state: %s", raft_state_.toString());

  return Status::OK;
}

Status RaftServiceImpl::RequestVoteReply(ServerContext *context,
                                         const RequestVoteResult *request,
                                         google::protobuf::Empty *reply) {
  utility::logInfo("Received reply: %s",
                   (request->votegranted() ? "granted" : "rejected"));
  // utility::logInfo("%d", request->term());
  // utility::logInfo("%p", &raft_state_);
  

  if (request->term() < raft_state_.currentTerm) {
    // Expired replies
    // Ignore
  } else if (request->term() == raft_state_.currentTerm) {
    if (request->votegranted()) {
      raft_state_.granted++;
      utility::logInfo("Visited");
    }

    switch (raft_state_.role_) {
    case RaftRole::RAFT_LEADER:
      // Already a leader, do not care about replies
      break;
    case RaftRole::RAFT_FOLLOWER:
      // No idea why we have this branch
      break;
    case RaftRole::RAFT_CANDIDATE:
      if ((raft_state_.granted + 1) * 2 > raft_state_.raft_size_) {
        // Getting majority, switch to leader
        raft_state_.switchToLeader();
      }
      break;
    default:
      throw std::runtime_error("Invalid role");
      break;
    }
  } else {
    // Getting reply from future term number. Is it even possible?
    throw std::runtime_error("Getting future term number");
    utility::logInfo("Impossible");
  }

  return Status::OK;
}

} // namespace application
