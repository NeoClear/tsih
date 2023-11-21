#pragma once

#include <grpcpp/grpcpp.h>

#include "absl/container/flat_hash_set.h"

#include "services/ping.grpc.pb.h"

using services::PingMessage;
using services::PingTracker;
using services::ServerIdentity;

using grpc::Channel;

namespace services {
// void PingerService(const absl::flat_hash_set<unsigned long long> &dests,
//                    const unsigned long long senderType,
//                    const unsigned long long senderPort);

class PingerClient {
public:
  PingerClient(const uint16_t master_count, const ServerIdentity identity);

  void ping();

private:
  absl::flat_hash_set<std::unique_ptr<PingTracker::Stub>> master_stubs_;

  const ServerIdentity identity_;
};

class Pinger {
public:
  virtual void pings() = 0;
};

} // namespace services