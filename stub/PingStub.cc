#include "stub/PingStub.h"
#include "utility/Logger.h"

#include "absl/container/flat_hash_set.h"

#include "config.h"

using grpc::ClientContext;
using token::PingMessage;

namespace stub {

PingStub::PingStub(const uint16_t master_count, const token::ServerType type,
                   const uint64_t candidateIdx)
    : type_(type), candidate_idx_(candidateIdx) {
  for (uint16_t i = 0; i < master_count; ++i) {
    // Never send to yourself
    if (type_ == token::ServerType::MASTER && candidate_idx_ == i) {
      continue;
    }

    std::shared_ptr<Channel> channel = grpc::CreateChannel(
        absl::StrFormat("localhost:%u", i + MASTER_BASE_PORT),
        grpc::InsecureChannelCredentials());
    master_stubs_.emplace_back(RaftService::NewStub(channel));
  }
}

void PingStub::ping() {
  PingMessage pingMsg;

  token::ServerIdentity identity;

  identity.set_server_type(type_);
  identity.set_server_index(candidate_idx_);

  pingMsg.mutable_server_identity()->CopyFrom(identity);

  std::vector<ClientContext> contexts(master_stubs_.size());
  google::protobuf::Empty empty;

  uint64_t finishedNum = 0;
  std::mutex mux;
  std::condition_variable cv;

  for (uint64_t i = 0; i < master_stubs_.size(); ++i) {
    master_stubs_[i]->async()->Ping(&contexts[i], &pingMsg, &empty,
                                    [&](grpc::Status status) {
                                      std::unique_lock lock(mux);
                                      finishedNum++;

                                      if (finishedNum == contexts.size()) {
                                        cv.notify_all();
                                      }
                                    });
  }

  std::unique_lock lock(mux);

  cv.wait(lock, [&contexts, &finishedNum]() {
    return finishedNum == contexts.size();
  });
}

} // namespace stub
