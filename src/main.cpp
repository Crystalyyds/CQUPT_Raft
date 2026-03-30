#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "raft/command.h"
#include "raft/config.h"
#include "raft/logging.h"
#include "raft/propose.h"
#include "raft/raft_node.h"

namespace raftdemo
{
  namespace
  {

    /**
     * @brief 将提案状态码转换成可读字符串。
     *
     * 这个函数只用于 main 中打印测试结果，方便观察 Propose 的执行情况。
     *
     * @param status 提案结果状态码
     * @return const char* 可读字符串
     */
    const char *ProposeStatusName(ProposeStatus status)
    {
      switch (status)
      {
      case ProposeStatus::kOk:
        return "Ok";
      case ProposeStatus::kNotLeader:
        return "NotLeader";
      case ProposeStatus::kInvalidCommand:
        return "InvalidCommand";
      case ProposeStatus::kNodeStopping:
        return "NodeStopping";
      case ProposeStatus::kReplicationFailed:
        return "ReplicationFailed";
      case ProposeStatus::kCommitFailed:
        return "CommitFailed";
      case ProposeStatus::kApplyFailed:
        return "ApplyFailed";
      case ProposeStatus::kTimeout:
        return "Timeout";
      }
      return "Unknown";
    }

    /**
     * @brief 打印当前集群快照。
     *
     * 通过调用每个节点的 Describe() 来输出当前角色、任期、提交位置等信息，
     * 用于观察选主和日志复制是否符合预期。
     *
     * @param title 本次快照标题
     * @param nodes 节点列表
     */
    void PrintClusterSnapshot(const std::string &title,
                              const std::vector<std::shared_ptr<RaftNode>> &nodes)
    {
      Log("main", title);
      for (const auto &node : nodes)
      {
        Log("main", node->Describe());
      }
    }

    /**
     * @brief 判断某个节点当前是否为 Leader。
     *
     * 当前为了不修改 RaftNode 的公有接口，
     * 暂时通过 Describe() 输出内容判断 role=Leader。
     *
     * 这只是测试代码里的临时做法。
     * 后续更工业化的写法建议给 RaftNode 增加只读查询接口。
     *
     * @param node 待检查节点
     * @return true  当前节点是 Leader
     * @return false 当前节点不是 Leader
     */
    bool IsLeaderNode(const std::shared_ptr<RaftNode> &node)
    {
      return node->Describe().find("role=Leader") != std::string::npos;
    }

    /**
     * @brief 在限定时间内等待集群选出 Leader。
     *
     * @param nodes 集群节点列表
     * @param timeout 最长等待时间
     * @return std::shared_ptr<RaftNode> 找到的 Leader；若超时未找到则返回空指针
     */
    std::shared_ptr<RaftNode> WaitForLeader(
        const std::vector<std::shared_ptr<RaftNode>> &nodes,
        std::chrono::milliseconds timeout)
    {
      const auto deadline = std::chrono::steady_clock::now() + timeout;

      while (std::chrono::steady_clock::now() < deadline)
      {
        for (const auto &node : nodes)
        {
          if (IsLeaderNode(node))
          {
            return node;
          }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }

      return nullptr;
    }

    /**
     * @brief 构造三节点测试配置。
     *
     * 这里创建一个最小三节点集群：
     * - node-1 -> 127.0.0.1:50051
     * - node-2 -> 127.0.0.1:50052
     * - node-3 -> 127.0.0.1:50053
     *
     * @return std::vector<NodeConfig> 三个节点的配置列表
     */
    std::vector<NodeConfig> BuildThreeNodeConfigs()
    {
      NodeConfig n1;
      n1.node_id = 1;
      n1.address = "127.0.0.1:50051";
      n1.peers = {
          PeerConfig{2, "127.0.0.1:50052"},
          PeerConfig{3, "127.0.0.1:50053"},
      };
      n1.election_timeout_min = std::chrono::milliseconds(300);
      n1.election_timeout_max = std::chrono::milliseconds(600);
      n1.heartbeat_interval = std::chrono::milliseconds(100);
      n1.rpc_deadline = std::chrono::milliseconds(500);

      NodeConfig n2;
      n2.node_id = 2;
      n2.address = "127.0.0.1:50052";
      n2.peers = {
          PeerConfig{1, "127.0.0.1:50051"},
          PeerConfig{3, "127.0.0.1:50053"},
      };
      n2.election_timeout_min = std::chrono::milliseconds(300);
      n2.election_timeout_max = std::chrono::milliseconds(600);
      n2.heartbeat_interval = std::chrono::milliseconds(100);
      n2.rpc_deadline = std::chrono::milliseconds(500);

      NodeConfig n3;
      n3.node_id = 3;
      n3.address = "127.0.0.1:50053";
      n3.peers = {
          PeerConfig{1, "127.0.0.1:50051"},
          PeerConfig{2, "127.0.0.1:50052"},
      };
      n3.election_timeout_min = std::chrono::milliseconds(300);
      n3.election_timeout_max = std::chrono::milliseconds(600);
      n3.heartbeat_interval = std::chrono::milliseconds(100);
      n3.rpc_deadline = std::chrono::milliseconds(500);

      return {n1, n2, n3};
    }

  } // namespace
} // namespace raftdemo

int main()
{
  using namespace raftdemo;

  // 1. 构造三节点配置
  const auto configs = BuildThreeNodeConfigs();

  // 2. 创建三个 RaftNode
  std::vector<std::shared_ptr<RaftNode>> nodes;
  nodes.reserve(configs.size());
  for (const auto &cfg : configs)
  {
    nodes.push_back(std::make_shared<RaftNode>(cfg));
  }

  // 3. 每个节点各自起一个线程，模拟三台主机独立运行
  std::vector<std::thread> threads;
  threads.reserve(nodes.size());
  for (const auto &node : nodes)
  {
    threads.emplace_back([node]()
                         {
      node->Start();
      node->Wait(); });
  }

  // 4. 等待 Leader 选举完成
  auto leader = WaitForLeader(nodes, std::chrono::milliseconds(5000));
  if (!leader)
  {
    Log("main", "failed to find leader within timeout");
    Log("main", "stop all nodes");

    for (auto &node : nodes)
    {
      node->Stop();
    }
    for (auto &t : threads)
    {
      if (t.joinable())
      {
        t.join();
      }
    }
    return 1;
  }

  // 5. 打印第一次集群快照，确认当前 Leader 和各节点状态
  PrintClusterSnapshot("cluster snapshot before propose:", nodes);

  // 6. 构造一条测试命令
  //
  // 当前先测试最简单的一条写命令：
  // SET x = 1
  Command cmd;
  cmd.type = CommandType::kSet;
  cmd.key = "x";
  cmd.value = "1";

  Log("main", "send command to leader: SET x 1");

  // 7. 调用 Leader 的 Propose()，测试写入链路
  const ProposeResult result = leader->Propose(cmd);

  Log("main",
      "propose result: status=", ProposeStatusName(result.status),
      ", leader_id=", result.leader_id,
      ", term=", result.term,
      ", log_index=", result.log_index,
      ", message=", result.message);

  // 8. 等待一小段时间，让 Leader 的后续心跳把新的 leader_commit 传播给 followers
  //
  // 说明：
  // 当前最小版本里，Leader 在本次复制成功后推进 commit_index_，
  // 但 followers 通常要靠下一轮 AppendEntries/心跳拿到新的 leader_commit。
  std::this_thread::sleep_for(std::chrono::milliseconds(800));

  // 9. 再打印一次快照，观察日志和提交位置是否变化
  PrintClusterSnapshot("cluster snapshot after propose:", nodes);

  // 10. 继续跑一小会儿，观察系统是否稳定
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  // 11. 停止所有节点
  Log("main", "stop all nodes");
  for (auto &node : nodes)
  {
    node->Stop();
  }

  // 12. 等待线程退出
  for (auto &t : threads)
  {
    if (t.joinable())
    {
      t.join();
    }
  }

  return result.Ok() ? 0 : 2;
}