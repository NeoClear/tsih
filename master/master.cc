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

void RunMaster(uint16_t index, uint16_t master_count) {
  std::string server_address =
      absl::StrFormat("0.0.0.0:%d", index + MASTER_BASE_PORT);
  application::MasterService service(index, master_count);

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;

  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

  builder.RegisterService(&service);

  std::unique_ptr<Server> server(builder.BuildAndStart());

  std::cout << "Server listening on " << server_address << std::endl;

  service.runPeriodicTasks();

  server->Wait();
}

ABSL_FLAG(uint16_t, idx, 0, "Index of this master");
ABSL_FLAG(uint16_t, master_count, 0, "Number of masters");

int main(int argc, char **argv) {
  absl::ParseCommandLine(argc, argv);

  RunMaster(absl::GetFlag(FLAGS_idx), absl::GetFlag(FLAGS_master_count));
  return 0;
}
