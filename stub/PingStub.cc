#include "stub/PingStub.h"
#include "utility/Logger.h"

#include "absl/container/flat_hash_set.h"

#include "config.h"

using grpc::ClientContext;
using token::PingMessage;

namespace stub {

PingStub::PingStub(
    const std::vector<std::unique_ptr<RaftService::Stub>>& raftStubs,
    token::ServerType type, uint64_t candidateIdx)
    : raft_stubs_(raftStubs), type_(type), candidate_idx_(candidateIdx) {}

void PingStub::ping() {
  PingMessage pingMsg;

  token::ServerIdentity identity;

  identity.set_server_type(type_);
  identity.set_server_index(candidate_idx_);

  pingMsg.mutable_server_identity()->CopyFrom(identity);

  std::vector<ClientContext> contexts(raft_stubs_.size());
  google::protobuf::Empty empty;

  uint64_t finishedNum = 0;
  std::mutex mux;
  std::condition_variable cv;

  uint64_t requestSize =
      type_ == token::MASTER ? contexts.size() - 1 : contexts.size();

  for (uint64_t i = 0; i < raft_stubs_.size(); ++i) {
    if (type_ == token::MASTER && i == candidate_idx_) {
      continue;
    }

    raft_stubs_[i]->async()->Ping(
        &contexts[i], &pingMsg, &empty,
        [requestSize, &mux, &finishedNum, &cv](grpc::Status status) {
          std::unique_lock lock(mux);
          finishedNum++;

          if (finishedNum == requestSize) {
            cv.notify_all();
          }
        });
  }

  std::unique_lock lock(mux);

  cv.wait(lock,
          [requestSize, &finishedNum]() { return finishedNum == requestSize; });
}

} // namespace stub
