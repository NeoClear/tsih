#pragma once

#include <iostream>
#include <memory>
#include <queue>
#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "proto/api.grpc.pb.h"

#include "stub/pinger.h"
#include "utility/periodic.h"
#include "utility/ping_history.h"

#include <google/protobuf/util/message_differencer.h>

#include "config.h"

using api::PingTracker;
using token::PingMessage;

using token::ServerIdentity;

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

namespace application {

class MasterService final : public PingTracker::Service,
                            public utility::Periodic {
public:
  MasterService(const uint16_t index, const uint16_t master_count);

  Status Ping(ServerContext* context, const PingMessage* request,
              google::protobuf::Empty* reply) override;

private:
  const uint16_t master_count_;
  const token::ServerIdentity identity_;
  utility::PingHistory ping_history_;

  static const token::ServerIdentity initializeIdentity(uint16_t index);

  void pingMasters();

  void updateLiveness();

  void setupPeriodicTasks();
};

} // namespace application
