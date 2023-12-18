#include <iostream>
#include <memory>
#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"

#include <grpcpp/grpcpp.h>

#include "config.h"

#include "proto/api.grpc.pb.h"

using api::PingTracker;
using api::RaftService;

using token::AppendEntriesArgument;
using token::AppendEntriesResult;
using token::PingMessage;
using token::RequestVoteArgument;
using token::RequestVoteResult;
using token::ServerIdentity;
using token::Task;

using grpc::Channel;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using grpc::ClientContext;

class TaskClient {
public:
  TaskClient(uint64_t masterSize) {
    for (uint64_t port = MASTER_BASE_PORT; port < MASTER_BASE_PORT + masterSize;
         ++port) {
      std::shared_ptr<Channel> channel =
          grpc::CreateChannel(absl::StrFormat("0.0.0.0:%u", port),
                              grpc::InsecureChannelCredentials());
      stubs_.emplace_back(RaftService::NewStub(channel));
    }
  }

  void addTask(const std::string &task) {
    Task request;
    request.set_value(task);

    google::protobuf::Empty empty;

    for (const std::unique_ptr<RaftService::Stub> &stub : stubs_) {
      ClientContext context;
      stub->AddTask(&context, request, &empty);
    }
  }

private:
  std::vector<std::unique_ptr<RaftService::Stub>> stubs_;
};

int main(int argc, char **argv) {
  TaskClient client(3);

  client.addTask("WHAT THE FUCK");

  return 0;
}
