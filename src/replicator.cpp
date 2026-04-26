#include "raft/replicator.h"

#include <algorithm>
#include <limits>
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

    std::uint64_t SafeAddOne(std::uint64_t value)
    {
      return value == std::numeric_limits<std::uint64_t>::max() ? value : value + 1;
    }
  } // namespace

  Replicator::Replicator(RaftNode &node, PeerConfig peer)
      : node_(node), peer_(std::move(peer))
  {
  }

  int Replicator::PeerId() const
  {
    return peer_.node_id;
  }

  bool Replicator::ReplicateOnce(std::uint64_t leader_term, std::uint64_t target_index,
                                 bool *should_apply)
  {
    std::lock_guard<std::mutex> replicator_lk(mu_);
    if (should_apply != nullptr)
    {
      *should_apply = false;
    }

    bool should_install_snapshot = false;
    raft::AppendEntriesRequest request;
    if (!BuildAppendEntriesRequest(leader_term, &request, &should_install_snapshot))
    {
      return false;
    }

    if (should_install_snapshot)
    {
      if (!node_.SendInstallSnapshotToPeer(peer_.node_id, leader_term))
      {
        return false;
      }

      if (target_index == 0)
      {
        return true;
      }

      std::lock_guard<std::mutex> lk(node_.mu_);
      const auto it = node_.match_index_.find(peer_.node_id);
      return it != node_.match_index_.end() && it->second >= target_index;
    }

    auto response = node_.AppendEntriesRpc(peer_.node_id, request);
    if (!response.has_value())
    {
      return false;
    }

    return HandleAppendEntriesResponse(leader_term, target_index, *response, should_apply);
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

    return false;
  }

} // namespace raftdemo
