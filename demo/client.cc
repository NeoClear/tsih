/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <iostream>
#include <memory>
#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"

#include <grpcpp/grpcpp.h>

#include "demo/demo.grpc.pb.h"

ABSL_FLAG(std::string, address, "localhost:50051", "Server address");

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using demo::Echoer;
using demo::EchoReply;
using demo::EchoRequest;

class EchoClient {
 public:
  EchoClient(std::shared_ptr<Channel> channel)
      : stub_(Echoer::NewStub(channel)) {}

  std::string echo(const std::string& user) {
    EchoRequest request;
    request.set_name(user);

    EchoReply reply;

    ClientContext context;

    Status status = stub_->Echo(&context, request, &reply);

    if (status.ok()) {
      return reply.msg();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }

 private:
  std::unique_ptr<Echoer::Stub> stub_;
};

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);
  std::string serverAddress = absl::GetFlag(FLAGS_address);

  EchoClient client(
      grpc::CreateChannel(serverAddress, grpc::InsecureChannelCredentials()));

  std::string reply = client.echo("Hello world!");
  std::cout << "Info received: " << reply << std::endl;

  return 0;
}
