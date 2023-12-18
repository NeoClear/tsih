#include "stub/PingStub.h"
#include "utility/Logger.h"

#include "absl/container/flat_hash_set.h"

#include "config.h"

using grpc::ClientContext;
using token::PingMessage;

namespace stub {

PingStub::PingStub(const uint16_t master_count, const ServerIdentity identity)
    : identity_(identity) {
  for (uint16_t i = 0; i < master_count; ++i) {
    // Never send to yourself
    if (identity.server_type() == token::ServerType::MASTER &&
        identity.server_index() == i) {
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

  pingMsg.mutable_server_identity()->CopyFrom(identity_);

  for (const auto& stub : master_stubs_) {
    ClientContext context;
    google::protobuf::Empty empty;

    grpc::Status status = stub->Ping(&context, pingMsg, &empty);
  }
}

} // namespace stub
