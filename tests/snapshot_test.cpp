#include <gtest/gtest.h>

#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "raft/common/command.h"
#include "raft/common/config.h"
#include "raft/runtime/logging.h"
#include "raft/common/propose.h"
#include "raft/node/raft_node.h"

namespace raftdemo
{
    namespace
    {

        using namespace std::chrono_literals;

        struct RunningCluster
        {
            RunningCluster() = default;
            RunningCluster(const RunningCluster &) = delete;
            RunningCluster &operator=(const RunningCluster &) = delete;

            RunningCluster(RunningCluster &&other) noexcept
                : nodes(std::move(other.nodes)), threads(std::move(other.threads)) {}

            RunningCluster &operator=(RunningCluster &&other) noexcept
            {
                if (this != &other)
                {
                    Stop();
                    nodes = std::move(other.nodes);
                    threads = std::move(other.threads);
                }
                return *this;
            }

            ~RunningCluster() { Stop(); }

            void Stop()
            {
                for (auto &node : nodes)
                {
                    if (node)
                    {
                        node->Stop();
                    }
                }
                for (auto &t : threads)
                {
                    if (t.joinable())
                    {
                        t.join();
                    }
                }
                threads.clear();
                nodes.clear();
            }

            std::vector<std::shared_ptr<RaftNode>> nodes;
            std::vector<std::thread> threads;
        };

        std::string GetEnvOrDefault(const char *key, const std::string &fallback)
        {
            const char *value = std::getenv(key);
            if (value == nullptr || *value == '\0')
            {
                return fallback;
            }
            return value;
        }

        std::filesystem::path TestBinaryDir()
        {
#ifdef RAFT_TEST_BINARY_DIR
            return std::filesystem::path(RAFT_TEST_BINARY_DIR);
#else
            return std::filesystem::current_path();
#endif
        }

        std::uint64_t NowForPath()
        {
            return static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count());
        }

        std::string UniqueRoot(const std::string &suffix)
        {
            const auto base = TestBinaryDir() / "raft_test_data" / "snapshot_recovery";
            std::filesystem::create_directories(base);
            return (base / (suffix + "_" + std::to_string(NowForPath()))).string();
        }

        std::vector<NodeConfig> BuildThreeNodeConfigs(const std::string &data_root, int base_port)
        {
            NodeConfig n1;
            n1.node_id = 1;
            n1.address = "127.0.0.1:" + std::to_string(base_port + 1);
            n1.peers = {
                PeerConfig{2, "127.0.0.1:" + std::to_string(base_port + 2)},
                PeerConfig{3, "127.0.0.1:" + std::to_string(base_port + 3)},
            };
            n1.election_timeout_min = 300ms;
            n1.election_timeout_max = 600ms;
            n1.heartbeat_interval = 80ms;
            n1.rpc_deadline = 500ms;
            n1.data_dir = (std::filesystem::path(data_root) / "node_1").string();

            NodeConfig n2;
            n2.node_id = 2;
            n2.address = "127.0.0.1:" + std::to_string(base_port + 2);
            n2.peers = {
                PeerConfig{1, "127.0.0.1:" + std::to_string(base_port + 1)},
                PeerConfig{3, "127.0.0.1:" + std::to_string(base_port + 3)},
            };
            n2.election_timeout_min = 300ms;
            n2.election_timeout_max = 600ms;
            n2.heartbeat_interval = 80ms;
            n2.rpc_deadline = 500ms;
            n2.data_dir = (std::filesystem::path(data_root) / "node_2").string();

            NodeConfig n3;
            n3.node_id = 3;
            n3.address = "127.0.0.1:" + std::to_string(base_port + 3);
            n3.peers = {
                PeerConfig{1, "127.0.0.1:" + std::to_string(base_port + 1)},
                PeerConfig{2, "127.0.0.1:" + std::to_string(base_port + 2)},
            };
            n3.election_timeout_min = 300ms;
            n3.election_timeout_max = 600ms;
            n3.heartbeat_interval = 80ms;
            n3.rpc_deadline = 500ms;
            n3.data_dir = (std::filesystem::path(data_root) / "node_3").string();

            return {n1, n2, n3};
        }

        std::vector<snapshotConfig> BuildSnapshotConfigs(const std::string &snapshot_root)
        {
            snapshotConfig s1;
            s1.enabled = true;
            s1.snapshot_dir = (std::filesystem::path(snapshot_root) / "node_1").string();
            s1.log_threshold = 20;
            s1.snapshot_interval = 10min;
            s1.max_snapshot_count = 5;
            s1.load_on_startup = true;
            s1.file_prefix = "snapshot";

            snapshotConfig s2 = s1;
            s2.snapshot_dir = (std::filesystem::path(snapshot_root) / "node_2").string();

            snapshotConfig s3 = s1;
            s3.snapshot_dir = (std::filesystem::path(snapshot_root) / "node_3").string();

            return {s1, s2, s3};
        }

        RunningCluster StartCluster(const std::vector<NodeConfig> &node_configs,
                                    const std::vector<snapshotConfig> &snapshot_configs)
        {
            RunningCluster cluster;
            cluster.nodes.reserve(node_configs.size());
            cluster.threads.reserve(node_configs.size());

            for (std::size_t i = 0; i < node_configs.size(); ++i)
            {
                cluster.nodes.push_back(std::make_shared<RaftNode>(node_configs[i], snapshot_configs[i]));
            }

            for (const auto &node : cluster.nodes)
            {
                cluster.threads.emplace_back([node]()
                                             {
      node->Start();
      node->Wait(); });
            }

            return cluster;
        }

        void StopCluster(RunningCluster *cluster)
        {
            if (cluster != nullptr)
            {
                cluster->Stop();
            }
        }

        bool IsLeaderNode(const std::shared_ptr<RaftNode> &node)
        {
            return node != nullptr && node->Describe().find("role=Leader") != std::string::npos;
        }

        std::shared_ptr<RaftNode> WaitForLeader(const std::vector<std::shared_ptr<RaftNode>> &nodes,
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
                std::this_thread::sleep_for(50ms);
            }
            return nullptr;
        }

        bool WaitForValueOnAllNodes(const std::vector<std::shared_ptr<RaftNode>> &nodes,
                                    const std::string &key,
                                    const std::string &expected_value,
                                    std::chrono::milliseconds timeout)
        {
            const auto deadline = std::chrono::steady_clock::now() + timeout;
            while (std::chrono::steady_clock::now() < deadline)
            {
                bool ok = true;
                for (const auto &node : nodes)
                {
                    std::string value;
                    if (!node->DebugGetValue(key, &value) || value != expected_value)
                    {
                        ok = false;
                        break;
                    }
                }
                if (ok)
                {
                    return true;
                }
                std::this_thread::sleep_for(100ms);
            }
            return false;
        }

        bool HasDirectorySnapshot(const snapshotConfig &cfg)
        {
            std::error_code ec;
            if (!std::filesystem::exists(cfg.snapshot_dir, ec))
            {
                return false;
            }

            for (const auto &entry : std::filesystem::directory_iterator(cfg.snapshot_dir, ec))
            {
                if (ec)
                {
                    return false;
                }
                const auto snapshot_dir = entry.path();
                const auto name = snapshot_dir.filename().string();
                if (!entry.is_directory(ec) || name.rfind(cfg.file_prefix + "_", 0) != 0)
                {
                    continue;
                }
                if (std::filesystem::exists(snapshot_dir / "data.bin", ec) &&
                    std::filesystem::exists(snapshot_dir / "__raft_snapshot_meta", ec))
                {
                    return true;
                }
            }
            return false;
        }

        bool WaitForSnapshots(const std::vector<snapshotConfig> &snapshot_configs,
                              std::chrono::milliseconds timeout)
        {
            const auto deadline = std::chrono::steady_clock::now() + timeout;
            const std::size_t required = snapshot_configs.size() / 2 + 1;
            while (std::chrono::steady_clock::now() < deadline)
            {
                std::size_t ready = 0;
                for (const auto &cfg : snapshot_configs)
                {
                    if (HasDirectorySnapshot(cfg))
                    {
                        ++ready;
                    }
                }
                if (ready >= required)
                {
                    return true;
                }
                std::this_thread::sleep_for(100ms);
            }
            return false;
        }

        Command MakeSet(const std::string &key, const std::string &value)
        {
            Command cmd;
            cmd.type = CommandType::kSet;
            cmd.key = key;
            cmd.value = value;
            return cmd;
        }

        void LogClusterState(const std::string &title,
                             const std::vector<std::shared_ptr<RaftNode>> &nodes)
        {
            Log("snapshot_recovery_gtest", title);
            for (const auto &node : nodes)
            {
                Log("snapshot_recovery_gtest", node->Describe());
            }
        }

        void LogSnapshotFiles(const std::vector<snapshotConfig> &snapshot_configs)
        {
            for (const auto &cfg : snapshot_configs)
            {
                std::error_code ec;
                Log("snapshot_recovery_gtest", "snapshot dir=", cfg.snapshot_dir);
                if (!std::filesystem::exists(cfg.snapshot_dir, ec))
                {
                    Log("snapshot_recovery_gtest", "  (not exists)");
                    continue;
                }
                for (const auto &entry : std::filesystem::recursive_directory_iterator(cfg.snapshot_dir, ec))
                {
                    if (ec)
                    {
                        break;
                    }
                    Log("snapshot_recovery_gtest", "  path=", entry.path().string());
                }
            }
        }

        TEST(RaftSnapshotRecoveryTest, SavesSnapshotAndRestoresAfterRestart)
        {
            const std::string data_root = GetEnvOrDefault(
                "RAFT_TEST_DATA_ROOT", UniqueRoot("snapshot_recovery_data"));
            const std::string snapshot_root = GetEnvOrDefault(
                "RAFT_TEST_SNAPSHOT_ROOT", UniqueRoot("snapshot_recovery_snapshots"));

            constexpr int kBasePort = 56050;

            std::filesystem::remove_all(data_root);
            std::filesystem::remove_all(snapshot_root);

            const auto node_configs = BuildThreeNodeConfigs(data_root, kBasePort);
            const auto snapshot_configs = BuildSnapshotConfigs(snapshot_root);

            Log("snapshot_recovery_gtest", "data_root=", data_root);
            Log("snapshot_recovery_gtest", "snapshot_root=", snapshot_root);
            Log("snapshot_recovery_gtest", "phase-1 start cluster and write logs");

            auto cluster = StartCluster(node_configs, snapshot_configs);
            auto leader = WaitForLeader(cluster.nodes, 6s);
            ASSERT_NE(leader, nullptr) << "leader not elected in phase-1";

            LogClusterState("phase-1 leader elected", cluster.nodes);

            for (int i = 0; i < 25; ++i)
            {
                const auto key = "k" + std::to_string(i);
                const auto value = "v" + std::to_string(i);
                const auto result = leader->Propose(MakeSet(key, value));
                ASSERT_TRUE(result.Ok()) << "propose failed at i=" << i << ", message=" << result.message;
            }

            ASSERT_TRUE(WaitForValueOnAllNodes(cluster.nodes, "k24", "v24", 8s))
                << "replicated values did not reach all nodes in phase-1";

            ASSERT_TRUE(WaitForSnapshots(snapshot_configs, 10s))
                << "snapshot files were not created for a majority of nodes";

            LogClusterState("phase-1 after 25 commands", cluster.nodes);
            LogSnapshotFiles(snapshot_configs);

            StopCluster(&cluster);

            Log("snapshot_recovery_gtest", "phase-2 restart cluster and verify recovery");

            auto restarted = StartCluster(node_configs, snapshot_configs);
            auto restarted_leader = WaitForLeader(restarted.nodes, 6s);
            ASSERT_NE(restarted_leader, nullptr) << "leader not elected after restart";

            LogClusterState("phase-2 after restart before new propose", restarted.nodes);

            ASSERT_TRUE(WaitForValueOnAllNodes(restarted.nodes, "k0", "v0", 8s))
                << "k0 was not restored on all nodes";
            ASSERT_TRUE(WaitForValueOnAllNodes(restarted.nodes, "k24", "v24", 8s))
                << "k24 was not restored on all nodes";

            const auto after_restart_result = restarted_leader->Propose(MakeSet("after_restart", "ok"));
            ASSERT_TRUE(after_restart_result.Ok()) << after_restart_result.message;

            ASSERT_TRUE(WaitForValueOnAllNodes(restarted.nodes, "after_restart", "ok", 8s))
                << "post-restart writes did not replicate to all nodes";

            LogClusterState("phase-2 after recovery and new propose", restarted.nodes);
            LogSnapshotFiles(snapshot_configs);

            StopCluster(&restarted);
        }

    } // namespace
} // namespace raftdemo
