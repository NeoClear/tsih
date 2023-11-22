#pragma once

#include <queue>

#include "absl/container/flat_hash_set.h"

#include "proto/token.grpc.pb.h"

namespace utility {

namespace detail {
using PingHistoryElement = std::pair<token::ServerIdentity, uint64_t>;
} // namespace detail

class PingHistory {
private:
  static uint64_t getCurrentMills() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
  }

  void removeOutdatedPings() {
    uint64_t currentTime = getCurrentMills();

    while (!ping_history_.empty() &&
           ((ping_history_.front().second + threshold_) < currentTime)) {
      ping_history_.pop_front();
    }
  }

public:
  explicit PingHistory(const uint64_t threshold) : threshold_(threshold) {}

  void addPing(const token::ServerIdentity identity) {
    std::unique_lock lock(mux_);

    removeOutdatedPings();

    ping_history_.emplace_back(identity, getCurrentMills());
  }

  absl::flat_hash_set<uint64_t> getRecentMasterIndices() {
    std::unique_lock lock(mux_);

    removeOutdatedPings();

    absl::flat_hash_set<uint64_t> masterIndices;

    for (auto identity : ping_history_) {
      if (identity.first.server_type() == token::MASTER) {
        masterIndices.insert(identity.first.server_index());
      }
    }

    return masterIndices;
  }

  absl::flat_hash_set<uint64_t> getRecentWorkerIndices() {
    std::unique_lock lock(mux_);

    removeOutdatedPings();

    absl::flat_hash_set<uint64_t> workerIndices;

    for (auto identity : ping_history_) {
      if (identity.first.server_type() == token::WORKER) {
        workerIndices.insert(identity.first.server_index());
      }
    }

    return workerIndices;
  }

private:
  std::deque<detail::PingHistoryElement> ping_history_;
  const uint64_t threshold_;
  std::mutex mux_;
};

} // namespace utility