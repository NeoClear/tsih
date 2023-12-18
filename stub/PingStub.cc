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
    master_stubs_.emplace(RaftService::NewStub(channel));
  }
}

void PingStub::ping() {
  PingMessage pingMsg;

  token::ServerIdentity identity;

  identity.set_server_type(type_);
  identity.set_server_index(candidate_idx_);

  pingMsg.mutable_server_identity()->CopyFrom(identity);

  for (const auto& stub : master_stubs_) {
    ClientContext context;
    google::protobuf::Empty empty;

    grpc::Status status = stub->Ping(&context, pingMsg, &empty);
  }
}

} // namespace stub
