#include "raft/replicator.h"

#include <algorithm>
#include <chrono>
#include <limits>
#include <sstream>
#include <string>
#include <utility>

#include "raft/logging.h"
#include "raft/raft_node.h"

namespace raftdemo
{
  namespace
  {
    constexpr std::size_t kMaxAppendEntriesPerRpc = 256;
    constexpr std::size_t kMaxAppendEntriesBytes = 512 * 1024;
    constexpr auto kInitialRetryBackoff = std::chrono::milliseconds(20);
    constexpr auto kMaxRetryBackoff = std::chrono::milliseconds(500);

    std::uint64_t SafeAddOne(std::uint64_t value)
    {
      return value == std::numeric_limits<std::uint64_t>::max() ? value : value + 1;
    }
  } // namespace

  Replicator::Replicator(RaftNode &node, PeerConfig peer)
      : node_(node), peer_(std::move(peer)), retry_backoff_(kInitialRetryBackoff)
  {
  }

  int Replicator::PeerId() const
  {
    return peer_.node_id;
  }

  bool Replicator::HasInflightRpc() const
  {
    std::lock_guard<std::mutex> lk(mu_);
    return append_inflight_ || snapshot_inflight_;
  }

  std::string Replicator::DebugString() const
  {
    std::lock_guard<std::mutex> lk(mu_);
    std::ostringstream oss;
    oss << "peer=" << peer_.node_id
        << ", append_inflight=" << append_inflight_
        << ", snapshot_inflight=" << snapshot_inflight_
        << ", backoff_ms=" << retry_backoff_.count()
        << ", append_rpc_started=" << append_rpc_started_
        << ", append_rpc_finished=" << append_rpc_finished_
        << ", snapshot_rpc_started=" << snapshot_rpc_started_
        << ", snapshot_rpc_finished=" << snapshot_rpc_finished_
        << ", transport_failures=" << transport_failures_;
    return oss.str();
  }

  bool Replicator::ReplicateOnce(std::uint64_t leader_term, std::uint64_t target_index,
                                 bool *should_apply)
  {
    if (should_apply != nullptr)
    {
      *should_apply = false;
    }

    raft::AppendEntriesRequest request;
    bool should_install_snapshot = false;

    {
      std::lock_guard<std::mutex> replicator_lk(mu_);

      if (IsTargetMatched(target_index))
      {
        return true;
      }

      // A single follower may have slow network or an unavailable server.  Do
      // not let heartbeats/proposes create concurrent RPCs to the same peer.
      if (append_inflight_ || snapshot_inflight_)
      {
        return false;
      }

      const auto now = std::chrono::steady_clock::now();
      if (IsBackoffActiveLocked(now))
      {
        return false;
      }

      if (!BuildAppendEntriesRequest(leader_term, &request, &should_install_snapshot))
      {
        return false;
      }

      if (should_install_snapshot)
      {
        snapshot_inflight_ = true;
        ++snapshot_rpc_started_;
      }
      else
      {
        append_inflight_ = true;
        ++append_rpc_started_;
      }
    }

    if (should_install_snapshot)
    {
      const bool ok = node_.SendInstallSnapshotToPeer(peer_.node_id, leader_term);

      {
        std::lock_guard<std::mutex> replicator_lk(mu_);
        FinishSnapshotRpcLocked();
        if (ok)
        {
          ResetBackoffLocked();
        }
        else
        {
          RecordTransportFailureLocked();
        }
      }

      if (target_index == 0)
      {
        return ok;
      }
      return ok && IsTargetMatched(target_index);
    }

    auto response = node_.AppendEntriesRpc(peer_.node_id, request);
    if (!response.has_value())
    {
      std::lock_guard<std::mutex> replicator_lk(mu_);
      FinishAppendRpcLocked();
      RecordTransportFailureLocked();
      return false;
    }

    bool matched = false;
    bool response_success = response->success();
    matched = HandleAppendEntriesResponse(leader_term, target_index, *response, should_apply);

    {
      std::lock_guard<std::mutex> replicator_lk(mu_);
      FinishAppendRpcLocked();
      // A successful AppendEntries or a valid rejection both mean the peer
      // responded and the replication state moved forward or backward
      // deliberately.  Only transport failures/backpressure use retry backoff.
      if (response_success)
      {
        ResetBackoffLocked();
      }
      else
      {
        next_retry_time_ = std::chrono::steady_clock::now();
      }
    }

    return matched;
  }

  bool Replicator::BuildAppendEntriesRequest(std::uint64_t leader_term,
                                             raft::AppendEntriesRequest *request,
                                             bool *should_install_snapshot)
  {
    if (request == nullptr || should_install_snapshot == nullptr)
    {
      return false;
    }

    *should_install_snapshot = false;

    std::lock_guard<std::mutex> lk(node_.mu_);
    if (!node_.running_.load() || node_.role_ != Role::kLeader ||
        node_.current_term_ != leader_term)
    {
      return false;
    }

    auto &next_index = node_.next_index_[peer_.node_id];
    if (next_index == 0)
    {
      next_index = SafeAddOne(node_.LastLogIndexLocked());
    }

    // If the follower needs a log entry earlier than the first log retained by
    // the leader, normal AppendEntries cannot repair it.  Install snapshot first.
    if (next_index <= node_.FirstLogIndexLocked())
    {
      *should_install_snapshot = true;
      return true;
    }

    const std::uint64_t prev_log_index = next_index - 1;
    const std::uint64_t prev_log_term = node_.TermAtIndexLocked(prev_log_index);
    if (prev_log_term == 0 && prev_log_index != 0)
    {
      *should_install_snapshot = true;
      return true;
    }

    request->set_term(leader_term);
    request->set_leader_id(node_.config_.node_id);
    request->set_prev_log_index(prev_log_index);
    request->set_prev_log_term(prev_log_term);
    request->set_leader_commit(node_.commit_index_);

    std::size_t entry_count = 0;
    std::size_t bytes_count = 0;
    for (std::uint64_t index = next_index;
         index <= node_.LastLogIndexLocked() && entry_count < kMaxAppendEntriesPerRpc;
         ++index)
    {
      const LogRecord *record = node_.LogAtIndexLocked(index);
      if (record == nullptr)
      {
        break;
      }

      const std::size_t next_bytes = bytes_count + record->command.size();
      if (entry_count > 0 && next_bytes > kMaxAppendEntriesBytes)
      {
        break;
      }

      auto *entry = request->add_entries();
      entry->set_index(record->index);
      entry->set_term(record->term);
      entry->set_command(record->command);
      bytes_count = next_bytes;
      ++entry_count;
    }

    return true;
  }

  bool Replicator::HandleAppendEntriesResponse(std::uint64_t leader_term,
                                               std::uint64_t target_index,
                                               const raft::AppendEntriesResponse &response,
                                               bool *should_apply)
  {
    std::lock_guard<std::mutex> lk(node_.mu_);
    if (!node_.running_.load())
    {
      return false;
    }

    if (response.term() > node_.current_term_)
    {
      node_.BecomeFollowerLocked(response.term(), -1,
                                 "peer replied higher term in AppendEntries");
      return false;
    }

    if (node_.role_ != Role::kLeader || node_.current_term_ != leader_term)
    {
      return false;
    }

    if (response.success())
    {
      auto &match_index = node_.match_index_[peer_.node_id];
      auto &next_index = node_.next_index_[peer_.node_id];
      match_index = std::max<std::uint64_t>(match_index, response.match_index());
      next_index = std::max<std::uint64_t>(next_index, SafeAddOne(response.match_index()));

      const std::uint64_t old_commit_index = node_.commit_index_;
      node_.AdvanceCommitIndexUnlocked();
      if (should_apply != nullptr && node_.commit_index_ > old_commit_index)
      {
        *should_apply = true;
      }

      return target_index == 0 || match_index >= target_index;
    }

    auto &next_index = node_.next_index_[peer_.node_id];
    const std::uint64_t hinted_next = SafeAddOne(response.last_log_index());
    if (hinted_next > 0 && hinted_next < next_index)
    {
      next_index = hinted_next;
    }
    else if (next_index > 1)
    {
      --next_index;
    }
    else
    {
      next_index = 1;
    }

    // Do not clamp next_index to FirstLogIndex here.  Leaving it below the
    // retained log boundary lets BuildAppendEntriesRequest switch to
    // InstallSnapshot on the next attempt.
    return false;
  }

  bool Replicator::IsTargetMatched(std::uint64_t target_index) const
  {
    if (target_index == 0)
    {
      return false;
    }

    std::lock_guard<std::mutex> lk(node_.mu_);
    const auto it = node_.match_index_.find(peer_.node_id);
    return it != node_.match_index_.end() && it->second >= target_index;
  }

  bool Replicator::IsBackoffActiveLocked(std::chrono::steady_clock::time_point now) const
  {
    return next_retry_time_ != std::chrono::steady_clock::time_point{} && now < next_retry_time_;
  }

  void Replicator::ResetBackoffLocked()
  {
    retry_backoff_ = kInitialRetryBackoff;
    next_retry_time_ = std::chrono::steady_clock::time_point{};
  }

  void Replicator::RecordTransportFailureLocked()
  {
    ++transport_failures_;
    const auto now = std::chrono::steady_clock::now();
    next_retry_time_ = now + retry_backoff_;
    retry_backoff_ = std::min(kMaxRetryBackoff, retry_backoff_ * 2);
  }

  void Replicator::FinishAppendRpcLocked()
  {
    append_inflight_ = false;
    ++append_rpc_finished_;
  }

  void Replicator::FinishSnapshotRpcLocked()
  {
    snapshot_inflight_ = false;
    ++snapshot_rpc_finished_;
  }

} // namespace raftdemo
