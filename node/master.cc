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

#include "application/RaftApplication.h"
#include "stub/PingStub.h"
#include "utility/Logger.h"
#include "utility/PingHistory.h"

#include <google/protobuf/util/message_differencer.h>

#include "config.h"

using api::PingTracker;
using token::PingMessage;

using token::ServerIdentity;

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

void RunMaster(uint64_t index, uint64_t master_count) {
  std::string server_address =
      absl::StrFormat("0.0.0.0:%d", index + MASTER_BASE_PORT);
  application::RaftServiceImpl service(master_count, index);

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;

  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

  builder.RegisterService(&service);

  std::unique_ptr<Server> server(builder.BuildAndStart());

  utility::logInfo("Server listening on %s", server_address);

  server->Wait();
}

ABSL_FLAG(uint16_t, idx, 0, "Index of this master");
ABSL_FLAG(uint16_t, master_count, 0, "Number of masters");

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);

  RunMaster(absl::GetFlag(FLAGS_idx), absl::GetFlag(FLAGS_master_count));
  return 0;
}
