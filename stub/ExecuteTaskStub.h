#pragma once

#include <grpcpp/grpcpp.h>
#include <vector>

#include "absl/container/flat_hash_set.h"

#include "proto/api.grpc.pb.h"

#include "config.h"

using api::RaftService;
using token::PingMessage;
using token::ServerIdentity;

using grpc::Channel;

namespace stub::ExecuteTaskStub {

bool executeTask(uint64_t workerIndex, uint64_t taskId,
                 std::string taskContent);

} // namespace stub::ExecuteTaskStub
