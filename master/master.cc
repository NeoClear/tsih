#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "absl/container/flat_hash_map.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "services/ping.grpc.pb.h"

#include "services/pinger.h"

#include "config.h"

using services::PingMessage;
using services::PingTracker;

using services::ServerIdentity;

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

class MasterService final : public PingTracker::Service,
                            public services::Pinger {
public:
  MasterService(const uint16_t index, const uint16_t master_count)
      : master_count_(master_count), identity_(initializeIdentity(index)) {}

  Status Ping(ServerContext *context, const PingMessage *request,
              google::protobuf::Empty *reply) override {
    const services::ServerIdentity &sender = request->server_identity();

    std::cout << "Received from "
              << services::ServerType_Name(sender.server_type()) << ", "
              << sender.server_index() << std::endl;
    return Status::OK;
  }

private:
  const uint16_t master_count_;
  const services::ServerIdentity identity_;

  static const services::ServerIdentity initializeIdentity(uint16_t index) {
    services::ServerIdentity identity;
    identity.set_server_type(services::MASTER);
    identity.set_server_index(index);

    return identity;
  }

public:
  /**
   * @brief Method that pings to all other masters
   * 
   */
  void pings() override {
    std::shared_ptr<services::PingerClient> client =
        std::make_shared<services::PingerClient>(master_count_, identity_);

    const auto pingLoop =
        [](std::shared_ptr<services::PingerClient> ping_client) {
          for (;;) {
            try {
              ping_client->ping();
              std::this_thread::sleep_for(
                  std::chrono::milliseconds(MASTER_PING_MILLS));
            } catch (...) {
              std::cerr << "Unable to ping" << std::endl;
            }
          }
        };

    try {
      std::thread pingThread(pingLoop, client);
      pingThread.detach();
    } catch (...) {
      std::cerr << "Unable to start ping" << std::endl;
    }
  }
};

void RunMaster(uint16_t index, uint16_t master_count) {
  std::string server_address =
      absl::StrFormat("0.0.0.0:%d", index + MASTER_BASE_PORT);
  MasterService service(index, master_count);

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;

  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

  builder.RegisterService(&service);

  std::unique_ptr<Server> server(builder.BuildAndStart());

  std::cout << "Server listening on " << server_address << std::endl;

  // Initiate a thread that periodically pings other servers
  service.pings();

  server->Wait();
}

ABSL_FLAG(uint16_t, idx, 0, "Index of this master");
ABSL_FLAG(uint16_t, master_count, 0, "Number of masters");

int main(int argc, char **argv) {
  absl::ParseCommandLine(argc, argv);

  RunMaster(absl::GetFlag(FLAGS_idx), absl::GetFlag(FLAGS_master_count));
  return 0;
}
