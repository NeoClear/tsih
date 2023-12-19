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
using token::SubmitTaskReply;
using token::SubmitTaskRequest;

using grpc::CallbackServerContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerUnaryReactor;
using grpc::Status;

namespace application {

class RaftServiceImpl : public api::RaftService::CallbackService {
public:
  explicit RaftServiceImpl(uint64_t raft_size, uint64_t candidate_idx)
      : raft_state_(raft_size, candidate_idx) {}

  ServerUnaryReactor* AppendEntries(CallbackServerContext* context,
                                    const AppendEntriesArgument* request,
                                    AppendEntriesResult* reply) override;

  ServerUnaryReactor* RequestVote(CallbackServerContext* context,
                                  const RequestVoteArgument* request,
                                  RequestVoteResult* reply) override;

  ServerUnaryReactor* Ping(CallbackServerContext* context,
                           const PingMessage* request,
                           google::protobuf::Empty* reply) override;

  ServerUnaryReactor* SubmitTask(CallbackServerContext* context,
                                 const SubmitTaskRequest* request,
                                 SubmitTaskReply* reply) override;

private:
  RaftState raft_state_;
};

} // namespace application
