#include <chrono>
#include <filesystem>
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

        std::vector<NodeConfig> BuildThreeNodeConfigs(const std::filesystem::path &data_root,
                                                      int base_port)
        {
            NodeConfig n1;
            n1.node_id = 1;
            n1.address = "127.0.0.1:" + std::to_string(base_port + 1);
            n1.peers = {
                PeerConfig{2, "127.0.0.1:" + std::to_string(base_port + 2)},
                PeerConfig{3, "127.0.0.1:" + std::to_string(base_port + 3)},
            };
            n1.election_timeout_min = std::chrono::milliseconds(300);
            n1.election_timeout_max = std::chrono::milliseconds(600);
            n1.heartbeat_interval = std::chrono::milliseconds(100);
            n1.rpc_deadline = std::chrono::milliseconds(500);
            n1.data_dir = (data_root / "demo_node_1").string();

            NodeConfig n2;
            n2.node_id = 2;
            n2.address = "127.0.0.1:" + std::to_string(base_port + 2);
            n2.peers = {
                PeerConfig{1, "127.0.0.1:" + std::to_string(base_port + 1)},
                PeerConfig{3, "127.0.0.1:" + std::to_string(base_port + 3)},
            };
            n2.election_timeout_min = std::chrono::milliseconds(300);
            n2.election_timeout_max = std::chrono::milliseconds(600);
            n2.heartbeat_interval = std::chrono::milliseconds(100);
            n2.rpc_deadline = std::chrono::milliseconds(500);
            n2.data_dir = (data_root / "demo_node_2").string();

            NodeConfig n3;
            n3.node_id = 3;
            n3.address = "127.0.0.1:" + std::to_string(base_port + 3);
            n3.peers = {
                PeerConfig{1, "127.0.0.1:" + std::to_string(base_port + 1)},
                PeerConfig{2, "127.0.0.1:" + std::to_string(base_port + 2)},
            };
            n3.election_timeout_min = std::chrono::milliseconds(300);
            n3.election_timeout_max = std::chrono::milliseconds(600);
            n3.heartbeat_interval = std::chrono::milliseconds(100);
            n3.rpc_deadline = std::chrono::milliseconds(500);
            n3.data_dir = (data_root / "demo_node_3").string();

            return {n1, n2, n3};
        }

        std::vector<snapshotConfig> BuildThreeSnapshotConfigs(const std::filesystem::path &snapshot_root)
        {
            snapshotConfig s1;
            s1.enabled = true;
            s1.snapshot_dir = (snapshot_root / "demo_node_1").string();
            s1.log_threshold = 20;
            s1.snapshot_interval = std::chrono::minutes(10);
            s1.max_snapshot_count = 5;
            s1.load_on_startup = true;
            s1.file_prefix = "snapshot";

            snapshotConfig s2 = s1;
            s2.snapshot_dir = (snapshot_root / "demo_node_2").string();

            snapshotConfig s3 = s1;
            s3.snapshot_dir = (snapshot_root / "demo_node_3").string();

            return {s1, s2, s3};
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

int main(int argc, char **argv)
{
    using namespace raftdemo;

    std::filesystem::path snapshot_root = "./raft_snapshots";
    std::filesystem::path data_root = "./raft_data";
    int base_port = 50050;

    if (argc >= 2)
    {
        snapshot_root = argv[1];
    }
    if (argc >= 3)
    {
        data_root = argv[2];
    }
    if (argc >= 4)
    {
        base_port = std::stoi(argv[3]);
    }

    Log("main", "cwd=", std::filesystem::current_path().string());
    Log("main", "snapshot_root=", snapshot_root.string());
    Log("main", "data_root=", data_root.string());
    Log("main", "base_port=", base_port);

    std::error_code ec;
    std::filesystem::create_directories(snapshot_root, ec);
    std::filesystem::create_directories(data_root, ec);

    const auto configs = BuildThreeNodeConfigs(data_root, base_port);
    const auto snapshot_configs = BuildThreeSnapshotConfigs(snapshot_root);

    std::vector<std::shared_ptr<RaftNode>> nodes;
    nodes.reserve(configs.size());
    for (std::size_t i = 0; i < configs.size(); ++i)
    {
        nodes.push_back(std::make_shared<RaftNode>(configs[i], snapshot_configs[i]));
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

    {
        Command cmd;
        cmd.type = CommandType::kSet;
        cmd.key = "x";
        cmd.value = "1";
        ok = ok && ProposeAndWait(leader, cmd, nodes,
                                  "SET x 1",
                                  "cluster snapshot after SET x 1:");
    }

    {
        Command cmd;
        cmd.type = CommandType::kSet;
        cmd.key = "y";
        cmd.value = "2";
        ok = ok && ProposeAndWait(leader, cmd, nodes,
                                  "SET y 2",
                                  "cluster snapshot after SET y 2:");
    }

    {
        Command cmd;
        cmd.type = CommandType::kSet;
        cmd.key = "x";
        cmd.value = "100";
        ok = ok && ProposeAndWait(leader, cmd, nodes,
                                  "SET x 100",
                                  "cluster snapshot after SET x 100:");
    }

    {
        Command cmd;
        cmd.type = CommandType::kDelete;
        cmd.key = "y";
        ok = ok && ProposeAndWait(leader, cmd, nodes,
                                  "DEL y",
                                  "cluster snapshot after DEL y:");
    }

    {
        Command cmd;
        cmd.type = CommandType::kSet;
        cmd.key = "z";
        cmd.value = "999";
        ok = ok && ProposeAndWait(leader, cmd, nodes,
                                  "SET z 999",
                                  "cluster snapshot after SET z 999:");
    }

    for (int i = 0; i < 25; ++i)
    {
        Command cmd;
        cmd.type = CommandType::kSet;
        cmd.key = "k" + std::to_string(i);
        cmd.value = "v" + std::to_string(i);

        ok = ok && ProposeAndWait(
                       leader, cmd, nodes,
                       "SET k" + std::to_string(i) + " v" + std::to_string(i),
                       "cluster snapshot after bulk write:");
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));
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

    Log("main", ok ? "demo finished successfully" : "demo finished with failures");
    return ok ? 0 : 2;
}