#include "stub/AppendStub.h"

namespace stub {

AppendStub::AppendStub(
    const std::vector<std::unique_ptr<RaftService::Stub>>& raftStubs)
    : raft_stubs_(raftStubs) {}

void AppendStub::sendAppendEntriesRequest(
    std::vector<bool> requestFilter, std::vector<ClientContext>& context,
    const std::vector<AppendEntriesArgument>& request,
    std::vector<AppendEntriesResult>& reply, std::vector<bool>& rpcStatus) {
  uint64_t requestCount =
      std::count(requestFilter.cbegin(), requestFilter.cend(), true);

  std::mutex mux;
  std::condition_variable cv;
  uint64_t respondedNum = 0;

  for (uint64_t i = 0; i < raft_stubs_.size(); ++i) {
    if (!requestFilter[i]) {
      continue;
    }

    raft_stubs_[i]->async()->AppendEntries(&context[i], &request[i], &reply[i],
                                           [&mux, i, &rpcStatus, &respondedNum,
                                            &cv,
                                            requestCount](grpc::Status status) {
                                             std::unique_lock lock(mux);
                                             rpcStatus[i] = status.ok();
                                             respondedNum++;

                                             if (respondedNum == requestCount) {
                                               cv.notify_all();
                                             }
                                           });
  }

  std::unique_lock lock(mux);
  cv.wait(lock, [&respondedNum, requestCount]() {
    return respondedNum == requestCount;
  });
}

} // namespace stub
