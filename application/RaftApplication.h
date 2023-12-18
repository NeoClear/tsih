#pragma once

#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <random>
#include <string>
#include <string_view>

#include "absl/container/flat_hash_map.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "proto/api.grpc.pb.h"

#include "application/RaftState.h"

#include "stub/PingStub.h"
#include "utility/Deadliner.h"
#include "utility/Logger.h"
#include "utility/PingHistory.h"

#include <google/protobuf/util/message_differencer.h>

#include "config.h"

using api::PingTracker;
using api::RaftService;

using token::AppendEntriesArgument;
using token::AppendEntriesResult;
using token::PingMessage;
using token::RequestVoteArgument;
using token::RequestVoteResult;
using token::ServerIdentity;
using token::Task;

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

namespace application {

class RaftServiceImpl : public api::RaftService::Service {
public:
  explicit RaftServiceImpl(uint64_t raft_size, uint64_t candidate_idx)
      : raft_state_(raft_size, candidate_idx) {}

  Status AppendEntriesRequest(ServerContext* context,
                              const AppendEntriesArgument* request,
                              google::protobuf::Empty* reply) override;
  Status AppendEntriesReply(ServerContext* context,
                            const AppendEntriesResult* request,
                            google::protobuf::Empty* reply) override;
  Status RequestVoteRequest(ServerContext* context,
                            const RequestVoteArgument* request,
                            google::protobuf::Empty* reply) override;
  Status RequestVoteReply(ServerContext* context,
                          const RequestVoteResult* request,
                          google::protobuf::Empty* reply) override;
  Status Ping(ServerContext* context, const PingMessage* request,
              google::protobuf::Empty* reply) override;
  Status AddTask(ServerContext* context, const Task* request,
                 google::protobuf::Empty* reply) override;

private:
  RaftState raft_state_;
};

} // namespace application
