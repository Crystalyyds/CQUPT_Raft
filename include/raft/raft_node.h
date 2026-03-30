#pragma once

#include <grpcpp/grpcpp.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "raft.pb.h"
#include "raft.grpc.pb.h"
#include "raft/config.h"
#include "raft/min_heap_timer.h"
#include "raft/thread_pool.h"
#include "raft/command.h"
#include "raft/propose.h"
#include "raft/state_machine.h"

namespace raftdemo
{

  class RaftServiceImpl;

  enum class Role
  {
    kFollower,
    kCandidate,
    kLeader,
  };

  struct LogRecord
  {
    std::uint64_t index;
    std::uint64_t term;
    std::string command;
  };

  class RaftNode : public std::enable_shared_from_this<RaftNode>
  {
  public:
    explicit RaftNode(NodeConfig config);
    ~RaftNode();

    void Start();
    void Stop();
    void Wait();

    void OnRequestVote(const raft::VoteRequest &request, raft::VoteResponse *response);
    void OnAppendEntries(const raft::AppendEntriesRequest &request,
                         raft::AppendEntriesResponse *response);

    std::string Describe() const;
    // 客户端提案入口。上层业务应通过这个接口把一条业务命令提交给当前节点。
    ProposeResult Propose(const Command &command);

  private:
    struct PeerClient
    {
      int peer_id{0};
      std::string address;
      std::shared_ptr<grpc::Channel> channel;
      std::unique_ptr<raft::RaftService::Stub> stub;
      std::mutex mu;
    };

    void InitServer();
    void InitClients();

    void ResetElectionTimerLocked();
    void ResetHeartbeatTimerLocked();
    std::chrono::milliseconds RandomElectionTimeoutLocked();

    void OnElectionTimeout();
    void StartElection();
    void OnElectionWon(std::uint64_t term);
    void SendHeartbeats();

    void BecomeFollowerLocked(std::uint64_t new_term, int new_leader, const std::string &reason);
    void BecomeLeaderLocked();

    bool IsCandidateLogUpToDateLocked(std::uint64_t last_log_index,
                                      std::uint64_t last_log_term) const;
    std::uint64_t LastLogIndexLocked() const;
    std::uint64_t LastLogTermLocked() const;

    std::optional<raft::VoteResponse> RequestVoteRpc(int peer_id, const raft::VoteRequest &request);
    std::optional<raft::AppendEntriesResponse> AppendEntriesRpc(
        int peer_id, const raft::AppendEntriesRequest &request);

    static const char *RoleName(Role role);
    // 在持锁条件下校验命令是否合法。这里只做命令本身的合法性检查
    bool ValidateCommandUnlocked(const Command &command, std::string *reason) const;
    // 在持锁条件下将命令追加到本地日志
    std::uint64_t AppendLocalLogUnlocked(const std::string &command_data);
    // 将指定日志项复制到多数派节点。
    bool ReplicateLogEntryToMajority(std::uint64_t log_index);
    // 在持锁条件下推进提交下标。
    void AdvanceCommitIndexUnlocked();
    // 将已提交但尚未执行的日志应用到状态机。
    void ApplyCommittedEntries();

    NodeConfig config_;

    mutable std::mutex mu_;
    Role role_{Role::kFollower};
    std::uint64_t current_term_{0};
    int voted_for_{-1};
    int leader_id_{-1};

    std::vector<LogRecord> log_;
    std::uint64_t commit_index_{0};
    std::uint64_t last_applied_{0};

    std::unordered_map<int, std::uint64_t> next_index_;
    std::unordered_map<int, std::uint64_t> match_index_;

    std::unordered_map<int, std::unique_ptr<PeerClient>> clients_;

    TimerScheduler scheduler_;
    ThreadPool rpc_pool_{4};
    std::optional<TimerScheduler::TaskId> election_timer_id_;
    std::optional<TimerScheduler::TaskId> heartbeat_timer_id_;

    std::mt19937 rng_;
    std::atomic<bool> running_{false};

    std::unique_ptr<RaftServiceImpl> service_;
    std::unique_ptr<grpc::Server> server_;
    // 当前节点绑定的状态机实例
    std::unique_ptr<IStateMachine> state_machine_;
    };

} // namespace raftdemo
