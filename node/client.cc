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
using token::SubmitTaskReply;
using token::SubmitTaskRequest;

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

  void submitTask(const std::string& task) {
    SubmitTaskRequest request;
    SubmitTaskReply reply;

    std::vector<ClientContext> contexts(stubs_.size());
    std::vector<SubmitTaskReply> replies(stubs_.size());

    request.set_value(task);

    std::mutex mux;
    std::condition_variable cv;

    uint64_t respondedNum = 0;

    for (uint64_t i = 0; i < stubs_.size(); ++i) {
      stubs_[i]->async()->SubmitTask(
          &contexts[i], &request, &replies[i],
          [&respondedNum, &mux, &cv, &replies, i](Status status) {
            std::unique_lock lock(mux);
            respondedNum++;

            if (replies[i].success()) {
              absl::PrintF("Task submitted!\n");
            }

            if (respondedNum == replies.size()) {
              cv.notify_all();
            }
          });
    }

    std::unique_lock lock(mux);
    cv.wait(lock, [&respondedNum, &replies]() {
      return respondedNum == replies.size();
    });
  }

private:
  std::vector<std::unique_ptr<RaftService::Stub>> stubs_;
};

ABSL_FLAG(uint16_t, master_count, 0, "Number of masters");

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);
  TaskClient client(absl::GetFlag(FLAGS_master_count));

  client.submitTask("Task");

  return 0;
}
