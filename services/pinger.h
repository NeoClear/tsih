#pragma once

#include <grpcpp/grpcpp.h>

#include "absl/container/flat_hash_set.h"

#include "services/ping.grpc.pb.h"

using services::PingMessage;
using services::PingTracker;
using services::ServerIdentity;

using grpc::Channel;

namespace services {
class PingerClient {
public:
  PingerClient(const uint16_t master_count, const ServerIdentity identity);

  void ping();

private:
  absl::flat_hash_set<std::unique_ptr<PingTracker::Stub>> master_stubs_;

  const ServerIdentity identity_;
};

} // namespace services