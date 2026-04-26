#pragma once

#include <cstdint>
#include <mutex>

#include "raft.pb.h"
#include "raft/config.h"

namespace raftdemo
{

  class RaftNode;

  class Replicator
  {
  public:
    Replicator(RaftNode &node, PeerConfig peer);

    int PeerId() const;

    // Replicate at most one batch to this follower. target_index == 0 means a
    // heartbeat/probe replication. When target_index > 0, the return value tells
    // whether this follower is known to have replicated at least target_index.
    bool ReplicateOnce(std::uint64_t leader_term, std::uint64_t target_index,
                       bool *should_apply);

  private:
    bool BuildAppendEntriesRequest(std::uint64_t leader_term,
                                   raft::AppendEntriesRequest *request,
                                   bool *should_install_snapshot);
    bool HandleAppendEntriesResponse(std::uint64_t leader_term,
                                     std::uint64_t target_index,
                                     const raft::AppendEntriesResponse &response,
                                     bool *should_apply);

    RaftNode &node_;
    PeerConfig peer_;
    std::mutex mu_;
  };

} // namespace raftdemo
