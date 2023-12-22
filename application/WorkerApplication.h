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

#include "stub/PingStub.h"
#include "utility/Deadliner.h"
#include "utility/Logger.h"
#include "utility/PingHistory.h"

#include <google/protobuf/util/message_differencer.h>

#include "config.h"

using token::ExecuteTaskReply;
using token::ExecuteTaskRequest;
using token::QueryTaskStatusReply;
using token::QueryTaskStatusRequest;
using token::QueryWorkerStatusReply;

using grpc::CallbackServerContext;
using grpc::ServerContext;
using grpc::ServerUnaryReactor;

namespace application {

class WorkerServiceImpl : public api::WorkerService::CallbackService {
public:
  explicit WorkerServiceImpl(uint64_t raftSize, uint64_t workerIndex);

  ServerUnaryReactor* ExecuteTask(CallbackServerContext* context,
                                  const ExecuteTaskRequest* request,
                                  ExecuteTaskReply* reply) override;

  ServerUnaryReactor* QueryWorkerStatus(CallbackServerContext* context,
                                        const google::protobuf::Empty* request,
                                        QueryWorkerStatusReply* reply) override;

  ServerUnaryReactor* QueryTaskStatus(CallbackServerContext* context,
                                      const QueryTaskStatusRequest* request,
                                      QueryTaskStatusReply* reply) override;

private:
  std::mutex mux_;

  const std::vector<std::unique_ptr<RaftService::Stub>> raft_stubs_;
  stub::PingStub ping_stub_;

  absl::flat_hash_set<uint64_t> running_task_ids_;

  utility::Deadliner ping_timeout_;
  void pingTimeoutCallback();

  std::vector<std::unique_ptr<RaftService::Stub>>
  initRaftStubs(uint64_t raftSize);
};

} // namespace application
