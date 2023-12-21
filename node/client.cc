#include <iostream>
#include <latch>
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

    request.set_value(task);
    bool submitted = false;

    for (uint64_t i = 0; i < stubs_.size(); ++i) {
      ClientContext context;
      SubmitTaskReply reply;

      std::latch onRequestCompleted(1);
      bool invoked = false;

      stubs_[i]->async()->SubmitTask(
          &context, &request, &reply,
          [&onRequestCompleted, &invoked](Status status) {
            if (status.ok()) {
              invoked = true;
            }
            onRequestCompleted.count_down();
          });
      onRequestCompleted.wait();

      if (invoked && reply.success()) {
        submitted = true;
        std::cout << "Job submitted: " << reply.jobid() << std::endl;
        break;
      }
    }

    if (!submitted) {
      std::cout << "Failed to submit job" << std::endl;
    }
  }

private:
  std::vector<std::unique_ptr<RaftService::Stub>> stubs_;
};

ABSL_FLAG(uint16_t, master_count, 0, "Number of masters");

std::vector<std::string> split(std::string_view view) {
  std::vector<std::string> pieces;

  for (char ch : view) {
    if (std::isspace(ch)) {
      // Do nothing if there is already a blank string at the end
      if (pieces.empty() || !pieces.back().empty()) {
        pieces.emplace_back();
      }
    } else {
      if (pieces.empty()) {
        pieces.emplace_back();
      }

      pieces.back().push_back(ch);
    }
  }

  if (!pieces.empty() && pieces.back().empty()) {
    pieces.pop_back();
  }

  return pieces;
}

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);
  TaskClient client(absl::GetFlag(FLAGS_master_count));

  std::string line;

  for (;;) {
    std::cout << "> ";
    if (!std::getline(std::cin, line)) {
      break;
    }

    auto pieces = split(line);

    if (pieces.empty()) {
      continue;
    } else if (pieces.size() == 1) {
      // Query worker count
      // Query pending jobs
      // Query running jobs
      // Query failed jobs
      // Query finished jobs
    } else if (pieces.size() == 2) {
      if (pieces[0] == "submit") {
        client.submitTask(pieces[1]);
      }
      // Case 1: query task_id
      // Case 2: submit task_code_filename
    }
  }

  return 0;
}
