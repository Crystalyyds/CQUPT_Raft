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

        void PrintClusterSnapshot(const std::string &title,
                                  const std::vector<std::shared_ptr<RaftNode>> &nodes)
        {
            Log("main", title);
            for (const auto &node : nodes)
            {
                Log("main", node->Describe());
            }
        }

        bool IsLeaderNode(const std::shared_ptr<RaftNode> &node)
        {
            return node->Describe().find("role=Leader") != std::string::npos;
        }

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
            n1.data_dir = "./raft_data/demo_node_1";

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
            n2.data_dir = "./raft_data/demo_node_2";

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
            n3.data_dir = "./raft_data/demo_node_3";

            return {n1, n2, n3};
        }

        bool ProposeAndWait(const std::shared_ptr<RaftNode> &leader,
                            const Command &cmd,
                            const std::vector<std::shared_ptr<RaftNode>> &nodes,
                            const std::string &command_desc,
                            const std::string &snapshot_title)
        {
            Log("main", "send command to leader: ", command_desc);

            const ProposeResult result = leader->Propose(cmd);

            Log("main",
                "propose result: status=", ProposeStatusName(result.status),
                ", leader_id=", result.leader_id,
                ", term=", result.term,
                ", log_index=", result.log_index,
                ", message=", result.message);

            std::this_thread::sleep_for(std::chrono::milliseconds(800));
            PrintClusterSnapshot(snapshot_title, nodes);
            return result.Ok();
        }

    } // namespace
} // namespace raftdemo

int main()
{
    using namespace raftdemo;

    const auto configs = BuildThreeNodeConfigs();

    std::vector<std::shared_ptr<RaftNode>> nodes;
    nodes.reserve(configs.size());
    for (const auto &cfg : configs)
    {
        nodes.push_back(std::make_shared<RaftNode>(cfg));
    }

    std::vector<std::thread> threads;
    threads.reserve(nodes.size());
    for (const auto &node : nodes)
    {
        threads.emplace_back([node]()
                             {
            node->Start();
            node->Wait(); });
    }

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

    PrintClusterSnapshot("cluster snapshot before propose:", nodes);

    bool ok = true;

    // 1. SET x 1
    {
        Command cmd;
        cmd.type = CommandType::kSet;
        cmd.key = "x";
        cmd.value = "1";
        ok = ok && ProposeAndWait(
                       leader, cmd, nodes,
                       "SET x 1",
                       "cluster snapshot after SET x 1:");
    }

    // 2. SET y 2
    {
        Command cmd;
        cmd.type = CommandType::kSet;
        cmd.key = "y";
        cmd.value = "2";
        ok = ok && ProposeAndWait(
                       leader, cmd, nodes,
                       "SET y 2",
                       "cluster snapshot after SET y 2:");
    }

    // 3. 覆盖写：SET x 100
    {
        Command cmd;
        cmd.type = CommandType::kSet;
        cmd.key = "x";
        cmd.value = "100";
        ok = ok && ProposeAndWait(
                       leader, cmd, nodes,
                       "SET x 100",
                       "cluster snapshot after SET x 100:");
    }

    // 4. 删除已有 key：DEL y
    {
        Command cmd;
        cmd.type = CommandType::kDelete;
        cmd.key = "y";
        cmd.value = "";
        ok = ok && ProposeAndWait(
                       leader, cmd, nodes,
                       "DEL y",
                       "cluster snapshot after DEL y:");
    }

    // 5. 删除不存在 key：DEL not_exist
    {
        Command cmd;
        cmd.type = CommandType::kDelete;
        cmd.key = "not_exist";
        cmd.value = "";
        ok = ok && ProposeAndWait(
                       leader, cmd, nodes,
                       "DEL not_exist",
                       "cluster snapshot after DEL not_exist:");
    }

    // 6. 新增 z
    {
        Command cmd;
        cmd.type = CommandType::kSet;
        cmd.key = "z";
        cmd.value = "999";
        ok = ok && ProposeAndWait(
                       leader, cmd, nodes,
                       "SET z 999",
                       "cluster snapshot after SET z 999:");
    }

    // 7. 删除 x
    {
        Command cmd;
        cmd.type = CommandType::kDelete;
        cmd.key = "x";
        cmd.value = "";
        ok = ok && ProposeAndWait(
                       leader, cmd, nodes,
                       "DEL x",
                       "cluster snapshot after DEL x:");
    }

    // 8. 再写回 x
    {
        Command cmd;
        cmd.type = CommandType::kSet;
        cmd.key = "x";
        cmd.value = "final";
        ok = ok && ProposeAndWait(
                       leader, cmd, nodes,
                       "SET x final",
                       "cluster snapshot after SET x final:");
    }

    // 9. 非法命令：空 key
    {
        Command cmd;
        cmd.type = CommandType::kSet;
        cmd.key = "";
        cmd.value = "bad";

        Log("main", "send invalid command to leader: SET <empty> bad");

        const ProposeResult result = leader->Propose(cmd);
        Log("main",
            "invalid propose result: status=", ProposeStatusName(result.status),
            ", leader_id=", result.leader_id,
            ", term=", result.term,
            ", log_index=", result.log_index,
            ", message=", result.message);
    }

    // 10. 向 follower 直接提案，预期 NotLeader
    std::shared_ptr<RaftNode> follower = nullptr;
    for (const auto &node : nodes)
    {
        if (node != leader)
        {
            follower = node;
            break;
        }
    }

    if (follower)
    {
        Command cmd;
        cmd.type = CommandType::kSet;
        cmd.key = "from_follower";
        cmd.value = "123";

        Log("main", "send command to follower directly: SET from_follower 123");

        const ProposeResult result = follower->Propose(cmd);
        Log("main",
            "follower propose result: status=", ProposeStatusName(result.status),
            ", leader_id=", result.leader_id,
            ", term=", result.term,
            ", log_index=", result.log_index,
            ", message=", result.message);
    }

    // 11. 连续多条命令测试
    for (int i = 0; i < 10; ++i)
    {
        Command cmd;
        cmd.type = CommandType::kSet;
        cmd.key = "k" + std::to_string(i);
        cmd.value = "v" + std::to_string(i);

        const std::string desc = "SET " + cmd.key + " " + cmd.value;
        const std::string snapshot = "cluster snapshot after " + desc + ":";

        ok = ok && ProposeAndWait(leader, cmd, nodes, desc, snapshot);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    PrintClusterSnapshot("cluster snapshot before stop:", nodes);

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

    return ok ? 0 : 2;
}
