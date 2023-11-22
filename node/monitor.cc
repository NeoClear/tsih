// #include <iostream>
// #include <memory>
// #include <string>
// #include <thread>

// #include "absl/container/flat_hash_map.h"
// #include "absl/flags/flag.h"
// #include "absl/flags/parse.h"
// #include "absl/strings/str_format.h"

// #include <grpcpp/ext/proto_server_reflection_plugin.h>
// #include <grpcpp/grpcpp.h>
// #include <grpcpp/health_check_service_interface.h>

// #include "services/api.grpc.pb.h"

// #include "services/pinger.h"

// using services::PingMessage;
// using services::PingTracker;

// using token::ServerIdentity;

// using grpc::Server;
// using grpc::ServerBuilder;
// using grpc::ServerContext;
// using grpc::Status;

// void dummy() {
//   token::ServerIdentity identity;
//   identity.set_server_type(services::ServerType::MASTER);
//   identity.set_server_index(0);

//   services::PingerStub client(3, identity);

//   auto pings = [&]() {
//     for (;;) {
//       try {
//         client.ping();
//         std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//       } catch (...) {
//         std::cout << "FFF..." << std::endl;
//       }
//     }
//   };

//   try {
//     std::thread t(pings);

//     t.join();
//   } catch (...) {
//     std::cout << "Something wrong..." << std::endl;
//   }
// }

// int main() {
//   dummy();
//   return 0;
// }
