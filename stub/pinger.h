#pragma once

#include <grpcpp/grpcpp.h>

#include "absl/container/flat_hash_set.h"

#include "proto/api.grpc.pb.h"

using api::PingTracker;
using token::PingMessage;
using token::ServerIdentity;

using grpc::Channel;

namespace stub {
class PingerStub {
public:
  PingerStub(const uint16_t master_count, const ServerIdentity identity);

  void ping();

private:
  absl::flat_hash_set<std::unique_ptr<PingTracker::Stub>> master_stubs_;

  const ServerIdentity identity_;
};

} // namespace stub
