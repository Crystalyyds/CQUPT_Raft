#include <gtest/gtest.h>

#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "raft/command.h"
#include "raft/config.h"
#include "raft/propose.h"
#include "raft/raft_node.h"

namespace raftdemo
{
  namespace
  {

    using namespace std::chrono_literals;
    namespace fs = std::filesystem;

    fs::path TestBinaryDir()
    {
#ifdef RAFT_TEST_BINARY_DIR
      return fs::path(RAFT_TEST_BINARY_DIR);
#else
      return fs::current_path();
#endif
    }

    std::uint64_t NowForPath()
    {
      return static_cast<std::uint64_t>(
          std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::system_clock::now().time_since_epoch())
              .count());
    }

    std::string SafeTestName()
    {
      const auto *info = ::testing::UnitTest::GetInstance()->current_test_info();
      std::string name = std::string(info->test_suite_name()) + "." + info->name();
      for (char &ch : name)
      {
        if (ch == '/' || ch == '\\' || ch == ':' || ch == ' ')
        {
          ch = '_';
        }
      }
      return name;
    }

    fs::path MakeTestRoot()
    {
      std::random_device rd;
      return TestBinaryDir() / "raft_test_data" / "log_replication" /
             (SafeTestName() + "_" + std::to_string(NowForPath()) + "_" +
              std::to_string(rd()));
    }

    int PickBasePort()
    {
      if (const char *env = std::getenv("RAFT_TEST_BASE_PORT"))
      {
        try
        {
          return std::stoi(env);
        }
        catch (...)
        {
        }
      }
      std::random_device rd;
      return 42000 + static_cast<int>(rd() % 13000);
    }

    bool IsLeaderSnapshot(const std::string &snapshot)
    {
      return snapshot.find("role=Leader") != std::string::npos;
    }

    bool ContainsAll(const std::string &snapshot, const std::vector<std::string> &parts)
    {
      for (const auto &part : parts)
      {
        if (snapshot.find(part) == std::string::npos)
        {
          return false;
        }
      }
      return true;
    }

    std::vector<NodeConfig> BuildThreeNodeConfigs(int base_port, const fs::path &root)
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
      n1.heartbeat_interval = std::chrono::milliseconds(80);
      n1.rpc_deadline = std::chrono::milliseconds(500);
      n1.data_dir = (root / "raft_data" / "node_1").string();

      NodeConfig n2;
      n2.node_id = 2;
      n2.address = "127.0.0.1:" + std::to_string(base_port + 2);
      n2.peers = {
          PeerConfig{1, "127.0.0.1:" + std::to_string(base_port + 1)},
          PeerConfig{3, "127.0.0.1:" + std::to_string(base_port + 3)},
      };
      n2.election_timeout_min = std::chrono::milliseconds(300);
      n2.election_timeout_max = std::chrono::milliseconds(600);
      n2.heartbeat_interval = std::chrono::milliseconds(80);
      n2.rpc_deadline = std::chrono::milliseconds(500);
      n2.data_dir = (root / "raft_data" / "node_2").string();

      NodeConfig n3;
      n3.node_id = 3;
      n3.address = "127.0.0.1:" + std::to_string(base_port + 3);
      n3.peers = {
          PeerConfig{1, "127.0.0.1:" + std::to_string(base_port + 1)},
          PeerConfig{2, "127.0.0.1:" + std::to_string(base_port + 2)},
      };
      n3.election_timeout_min = std::chrono::milliseconds(300);
      n3.election_timeout_max = std::chrono::milliseconds(600);
      n3.heartbeat_interval = std::chrono::milliseconds(80);
      n3.rpc_deadline = std::chrono::milliseconds(500);
      n3.data_dir = (root / "raft_data" / "node_3").string();

      return {n1, n2, n3};
    }

    class ClusterRunner
    {
    public:
      explicit ClusterRunner(int base_port) : root_(MakeTestRoot())
      {
        std::error_code ec;
        fs::remove_all(root_, ec);
        fs::create_directories(root_, ec);

        const auto configs = BuildThreeNodeConfigs(base_port, root_);
        snapshot_config_.enabled = false;
        snapshot_config_.snapshot_dir = (root_ / "raft_snapshots").string();

        nodes_.reserve(configs.size());
        for (const auto &cfg : configs)
        {
          nodes_.push_back(std::make_shared<RaftNode>(cfg, snapshot_config_));
        }
      }

      ~ClusterRunner() { Stop(); }

      void Start()
      {
        threads_.reserve(nodes_.size());
        for (const auto &node : nodes_)
        {
          threads_.emplace_back([node]()
                                {
        node->Start();
        node->Wait(); });
        }
      }

      void Stop()
      {
        for (auto &node : nodes_)
        {
          if (node)
          {
            node->Stop();
          }
        }
        for (auto &t : threads_)
        {
          if (t.joinable())
          {
            t.join();
          }
        }
        threads_.clear();
      }

      std::shared_ptr<RaftNode> WaitForLeader(std::chrono::milliseconds timeout) const
      {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline)
        {
          for (const auto &node : nodes_)
          {
            if (IsLeaderSnapshot(node->Describe()))
            {
              return node;
            }
          }
          std::this_thread::sleep_for(50ms);
        }
        return nullptr;
      }

      bool WaitUntilAll(const std::vector<std::string> &required_parts,
                        std::chrono::milliseconds timeout) const
      {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline)
        {
          bool ok = true;
          for (const auto &node : nodes_)
          {
            if (!ContainsAll(node->Describe(), required_parts))
            {
              ok = false;
              break;
            }
          }
          if (ok)
          {
            return true;
          }
          std::this_thread::sleep_for(50ms);
        }
        return false;
      }

    private:
      fs::path root_;
      snapshotConfig snapshot_config_;
      std::vector<std::shared_ptr<RaftNode>> nodes_;
      std::vector<std::thread> threads_;
    };

    TEST(RaftLogReplicationTest, LeaderProposeReplicatesLogToAllNodes)
    {
      ClusterRunner cluster(PickBasePort());
      cluster.Start();

      auto leader = cluster.WaitForLeader(5s);
      ASSERT_NE(leader, nullptr);

      Command cmd;
      cmd.type = CommandType::kSet;
      cmd.key = "x";
      cmd.value = "1";

      const ProposeResult result = leader->Propose(cmd);
      ASSERT_EQ(result.status, ProposeStatus::kOk) << result.message;
      ASSERT_GT(result.log_index, 0u);

      const std::string index = std::to_string(result.log_index);
      ASSERT_TRUE(cluster.WaitUntilAll({"last_log_index=" + index,
                                        "commit_index=" + index,
                                        "last_applied=" + index},
                                       5s));
      ASSERT_TRUE(cluster.WaitUntilAll({"kv={x=1}"}, 5s));
    }

    TEST(RaftLogReplicationTest, MultipleSequentialEntriesStayConsistentAcrossCluster)
    {
      ClusterRunner cluster(PickBasePort());
      cluster.Start();

      auto leader = cluster.WaitForLeader(5s);
      ASSERT_NE(leader, nullptr);

      auto propose_set = [&](const std::string &key, const std::string &value)
      {
        Command cmd;
        cmd.type = CommandType::kSet;
        cmd.key = key;
        cmd.value = value;
        return leader->Propose(cmd);
      };

      const ProposeResult r1 = propose_set("x", "1");
      ASSERT_EQ(r1.status, ProposeStatus::kOk) << r1.message;
      const ProposeResult r2 = propose_set("y", "2");
      ASSERT_EQ(r2.status, ProposeStatus::kOk) << r2.message;
      const ProposeResult r3 = propose_set("x", "100");
      ASSERT_EQ(r3.status, ProposeStatus::kOk) << r3.message;

      const std::string index = std::to_string(r3.log_index);
      ASSERT_TRUE(cluster.WaitUntilAll({"last_log_index=" + index,
                                        "commit_index=" + index,
                                        "last_applied=" + index},
                                       5s));
      ASSERT_TRUE(cluster.WaitUntilAll({"kv={x=100, y=2}"}, 5s));
    }

  } // namespace
} // namespace raftdemo
