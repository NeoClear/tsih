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

#include "services/ping.grpc.pb.h"

#include "master/master_application.h"
#include "services/periodic.h"
#include "services/ping_history.h"
#include "services/pinger.h"

#include <google/protobuf/util/message_differencer.h>

#include "config.h"

using services::PingMessage;
using services::PingTracker;

using services::ServerIdentity;

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

namespace application {

MasterService::MasterService(const uint16_t index, const uint16_t master_count)
    : master_count_(master_count), identity_(initializeIdentity(index)),
      ping_history_(2000) {
  setupPeriodicTasks();
}

Status MasterService::Ping(ServerContext *context, const PingMessage *request,
                           google::protobuf::Empty *reply) {
  services::ServerIdentity sender = request->server_identity();

  ping_history_.addPing(sender);

  return Status::OK;
}

const services::ServerIdentity
MasterService::initializeIdentity(uint16_t index) {
  services::ServerIdentity identity;
  identity.set_server_type(services::MASTER);
  identity.set_server_index(index);

  return identity;
}

void MasterService::pingMasters() {
  std::shared_ptr<services::PingerClient> client =
      std::make_shared<services::PingerClient>(master_count_, identity_);

  client->ping();
}

void MasterService::updateLiveness() {
  for (uint64_t masterIndex : ping_history_.getRecentMasterIndices()) {
    std::cout << "Master " << masterIndex << std::endl;
  }
}

void MasterService::setupPeriodicTasks() {
  addPeriodicTask(std::bind(&MasterService::pingMasters, this), 500);
  addPeriodicTask(std::bind(&MasterService::updateLiveness, this), 2000);
}

} // namespace application