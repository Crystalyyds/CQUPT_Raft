#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "raft/common/command.h"
#include "raft/common/config.h"
#include "raft/common/propose.h"
#include "raft/node/raft_node.h"

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

    bool KeepTestData()
    {
      const char *value = std::getenv("RAFT_TEST_KEEP_DATA");
      return value != nullptr && std::string(value) == "1";
    }

    int RandomBasePort()
    {
      // Keep ports in a high range and leave enough room for +1/+2/+3.
      // This avoids collisions with the older fixed-port tests when running a large test set.
      static std::random_device rd;
      static std::mt19937 rng(rd());
      std::uniform_int_distribution<int> dist(24000, 52000);
      return dist(rng);
    }

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
      }

      std::vector<std::shared_ptr<RaftNode>> nodes;
      std::vector<std::thread> threads;
    };

    class ScopedDataDir
    {
    public:
      explicit ScopedDataDir(std::string name)
      {
        std::random_device rd;
        std::error_code ec;
        root_ = TestBinaryDir() / "raft_test_data" / "persistence" /
                (std::move(name) + "_" + std::to_string(NowForPath()) + "_" +
                 std::to_string(rd()));
        fs::remove_all(root_, ec);
        ec.clear();
        fs::create_directories(root_, ec);
      }

      ~ScopedDataDir()
      {
        if (KeepTestData())
        {
          std::cout << "preserved test root: " << root_ << std::endl;
          return;
        }

        std::error_code ec;
        fs::remove_all(root_, ec);
      }

      const fs::path &path() const { return root_; }

    private:
      fs::path root_;
    };

    std::string DescribeAllNodes(const std::vector<std::shared_ptr<RaftNode>> &nodes)
    {
      std::ostringstream oss;
      for (std::size_t i = 0; i < nodes.size(); ++i)
      {
        oss << "node[" << i << "] ";
        if (!nodes[i])
        {
          oss << "<null>\n";
        }
        else
        {
          oss << nodes[i]->Describe() << "\n";
        }
      }
      return oss.str();
    }

    bool IsLeaderNode(const std::shared_ptr<RaftNode> &node)
    {
      return node != nullptr && node->Describe().find("role=Leader") != std::string::npos;
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
        std::this_thread::sleep_for(100ms);
      }
      return nullptr;
    }

    std::vector<NodeConfig> BuildThreeNodeConfigs(const fs::path &data_root, int base_port)
    {
      std::vector<NodeConfig> configs(3);

      for (int i = 0; i < 3; ++i)
      {
        const int node_id = i + 1;
        NodeConfig cfg;
        cfg.node_id = node_id;
        cfg.address = "127.0.0.1:" + std::to_string(base_port + node_id);
        cfg.election_timeout_min = std::chrono::milliseconds(700);
        cfg.election_timeout_max = std::chrono::milliseconds(1400);
        cfg.heartbeat_interval = std::chrono::milliseconds(120);
        cfg.rpc_deadline = std::chrono::milliseconds(700);
        cfg.data_dir = (data_root / ("node_" + std::to_string(node_id))).string();

        for (int peer_id = 1; peer_id <= 3; ++peer_id)
        {
          if (peer_id == node_id)
          {
            continue;
          }
          cfg.peers.push_back(
              PeerConfig{peer_id, "127.0.0.1:" + std::to_string(base_port + peer_id)});
        }

        configs[i] = std::move(cfg);
      }

      return configs;
    }

    snapshotConfig BuildPersistenceSnapshotConfig(const fs::path &root, int node_id)
    {
      snapshotConfig snapshot_config;
      snapshot_config.enabled = false;
      snapshot_config.load_on_startup = false;
      snapshot_config.snapshot_dir = (root / "snapshots" / ("node_" + std::to_string(node_id))).string();
      return snapshot_config;
    }

    RunningCluster StartCluster(const std::vector<NodeConfig> &configs, const fs::path &test_root)
    {
      RunningCluster cluster;
      cluster.nodes.reserve(configs.size());

      for (const auto &cfg : configs)
      {
        cluster.nodes.push_back(
            std::make_shared<RaftNode>(cfg, BuildPersistenceSnapshotConfig(test_root, cfg.node_id)));
      }

      cluster.threads.reserve(cluster.nodes.size());
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

    bool ProposeSetToLeader(const std::shared_ptr<RaftNode> &leader,
                            const std::string &key,
                            const std::string &value)
    {
      if (leader == nullptr)
      {
        return false;
      }

      Command cmd;
      cmd.type = CommandType::kSet;
      cmd.key = key;
      cmd.value = value;

      const ProposeResult result = leader->Propose(cmd);
      return result.Ok();
    }

    bool ProposeSetWithRetry(const std::vector<std::shared_ptr<RaftNode>> &nodes,
                             const std::string &key,
                             const std::string &value,
                             std::chrono::milliseconds timeout)
    {
      const auto deadline = std::chrono::steady_clock::now() + timeout;
      while (std::chrono::steady_clock::now() < deadline)
      {
        auto leader = WaitForLeader(nodes, 500ms);
        if (leader != nullptr && ProposeSetToLeader(leader, key, value))
        {
          return true;
        }
        std::this_thread::sleep_for(100ms);
      }
      return false;
    }

    bool WaitUntilValue(const std::vector<std::shared_ptr<RaftNode>> &nodes,
                        const std::string &key,
                        const std::string &expected_value,
                        std::chrono::milliseconds timeout)
    {
      const auto deadline = std::chrono::steady_clock::now() + timeout;
      while (std::chrono::steady_clock::now() < deadline)
      {
        bool all_ok = true;
        for (const auto &node : nodes)
        {
          if (node == nullptr)
          {
            all_ok = false;
            break;
          }

          std::string actual;
          if (!node->DebugGetValue(key, &actual) || actual != expected_value)
          {
            all_ok = false;
            break;
          }
        }

        if (all_ok)
        {
          return true;
        }

        std::this_thread::sleep_for(100ms);
      }

      return false;
    }

    bool ExtractIntField(const std::string &description,
                         const std::string &field_name,
                         int *value)
    {
      if (value == nullptr)
      {
        return false;
      }

      const std::string needle = field_name + "=";
      const std::size_t start = description.find(needle);
      if (start == std::string::npos)
      {
        return false;
      }

      std::size_t pos = start + needle.size();
      if (pos < description.size() && description[pos] == '-')
      {
        ++pos;
      }

      std::size_t end = pos;
      while (end < description.size() &&
             description[end] >= '0' &&
             description[end] <= '9')
      {
        ++end;
      }
      if (end == pos)
      {
        return false;
      }

      try
      {
        *value = std::stoi(description.substr(start + needle.size(),
                                              end - (start + needle.size())));
        return true;
      }
      catch (...)
      {
        return false;
      }
    }

    TEST(PersistenceTest, FullClusterRestartRecovery)
    {
      ScopedDataDir scoped_dir("test_full_restart");
      const int base_port = RandomBasePort();
      auto configs = BuildThreeNodeConfigs(scoped_dir.path() / "raft_data", base_port);

      auto cluster = StartCluster(configs, scoped_dir.path());
      ASSERT_NE(WaitForLeader(cluster.nodes, 10s), nullptr)
          << DescribeAllNodes(cluster.nodes);

      ASSERT_TRUE(ProposeSetWithRetry(cluster.nodes, "alpha", "1", 10s))
          << DescribeAllNodes(cluster.nodes);
      ASSERT_TRUE(ProposeSetWithRetry(cluster.nodes, "beta", "2", 10s))
          << DescribeAllNodes(cluster.nodes);

      ASSERT_TRUE(WaitUntilValue(cluster.nodes, "alpha", "1", 10s))
          << DescribeAllNodes(cluster.nodes);
      ASSERT_TRUE(WaitUntilValue(cluster.nodes, "beta", "2", 10s))
          << DescribeAllNodes(cluster.nodes);

      StopCluster(&cluster);
      std::this_thread::sleep_for(1200ms);

      cluster = StartCluster(configs, scoped_dir.path());
      ASSERT_NE(WaitForLeader(cluster.nodes, 10s), nullptr)
          << DescribeAllNodes(cluster.nodes);

      EXPECT_TRUE(WaitUntilValue(cluster.nodes, "alpha", "1", 12s))
          << DescribeAllNodes(cluster.nodes);
      EXPECT_TRUE(WaitUntilValue(cluster.nodes, "beta", "2", 12s))
          << DescribeAllNodes(cluster.nodes);

      StopCluster(&cluster);
    }

    TEST(PersistenceTest, RestartedFollowerCatchesUp)
    {
      ScopedDataDir scoped_dir("test_follower_restart");
      const int base_port = RandomBasePort();
      auto configs = BuildThreeNodeConfigs(scoped_dir.path() / "raft_data", base_port);

      auto cluster = StartCluster(configs, scoped_dir.path());
      auto leader = WaitForLeader(cluster.nodes, 10s);
      ASSERT_NE(leader, nullptr) << DescribeAllNodes(cluster.nodes);

      std::shared_ptr<RaftNode> follower;
      for (const auto &node : cluster.nodes)
      {
        if (node != leader)
        {
          follower = node;
          break;
        }
      }
      ASSERT_NE(follower, nullptr) << DescribeAllNodes(cluster.nodes);

      ASSERT_TRUE(ProposeSetWithRetry(cluster.nodes, "first", "100", 10s))
          << DescribeAllNodes(cluster.nodes);
      ASSERT_TRUE(WaitUntilValue(cluster.nodes, "first", "100", 10s))
          << DescribeAllNodes(cluster.nodes);

      follower->Stop();
      std::this_thread::sleep_for(1200ms);

      std::vector<std::shared_ptr<RaftNode>> alive_nodes;
      for (const auto &node : cluster.nodes)
      {
        if (node != follower)
        {
          alive_nodes.push_back(node);
        }
      }

      ASSERT_TRUE(ProposeSetWithRetry(alive_nodes, "second", "200", 10s))
          << DescribeAllNodes(cluster.nodes);
      ASSERT_TRUE(WaitUntilValue(alive_nodes, "second", "200", 10s))
          << DescribeAllNodes(cluster.nodes);

      const auto follower_it = std::find(cluster.nodes.begin(), cluster.nodes.end(), follower);
      ASSERT_NE(follower_it, cluster.nodes.end());

      const std::size_t follower_index =
          static_cast<std::size_t>(std::distance(cluster.nodes.begin(), follower_it));

      if (cluster.threads[follower_index].joinable())
      {
        cluster.threads[follower_index].join();
      }

      auto restarted_follower = std::make_shared<RaftNode>(
          configs[follower_index],
          BuildPersistenceSnapshotConfig(scoped_dir.path(), configs[follower_index].node_id));
      cluster.nodes[follower_index] = restarted_follower;
      cluster.threads[follower_index] = std::thread([restarted_follower]()
                                                    {
    restarted_follower->Start();
    restarted_follower->Wait(); });

      EXPECT_TRUE(WaitUntilValue(cluster.nodes, "first", "100", 12s))
          << DescribeAllNodes(cluster.nodes);
      EXPECT_TRUE(WaitUntilValue(cluster.nodes, "second", "200", 12s))
          << DescribeAllNodes(cluster.nodes);

      StopCluster(&cluster);
    }

    TEST(PersistenceTest, ColdRestartPreservesPersistedHardStateBeforeStart)
    {
      ScopedDataDir scoped_dir("test_hard_state_cold_restart");

      NodeConfig config;
      config.node_id = 1;
      config.address = "127.0.0.1:" + std::to_string(RandomBasePort());
      config.election_timeout_min = std::chrono::milliseconds(250);
      config.election_timeout_max = std::chrono::milliseconds(400);
      config.heartbeat_interval = std::chrono::milliseconds(80);
      config.rpc_deadline = std::chrono::milliseconds(500);
      config.data_dir = (scoped_dir.path() / "raft_data" / "node_1").string();

      const auto snapshot_config = BuildPersistenceSnapshotConfig(scoped_dir.path(), config.node_id);
      auto node = std::make_shared<RaftNode>(config, snapshot_config);
      std::thread worker([node]()
                         {
        node->Start();
        node->Wait(); });

      ASSERT_NE(WaitForLeader({node}, 8s), nullptr) << node->Describe();
      ASSERT_TRUE(ProposeSetWithRetry({node}, "hard_state_alpha", "1", 6s)) << node->Describe();
      ASSERT_TRUE(ProposeSetWithRetry({node}, "hard_state_beta", "2", 6s)) << node->Describe();
      ASSERT_TRUE(WaitUntilValue({node}, "hard_state_alpha", "1", 6s)) << node->Describe();
      ASSERT_TRUE(WaitUntilValue({node}, "hard_state_beta", "2", 6s)) << node->Describe();

      const NodeStatusSnapshot before_status = node->GetStatusSnapshot();
      const std::string before_description = node->Describe();
      int before_voted_for = -1;
      ASSERT_TRUE(ExtractIntField(before_description, "voted_for", &before_voted_for))
          << before_description;

      node->Stop();
      if (worker.joinable())
      {
        worker.join();
      }

      auto restarted = std::make_shared<RaftNode>(config, snapshot_config);
      const NodeStatusSnapshot after_status = restarted->GetStatusSnapshot();
      const std::string after_description = restarted->Describe();
      int after_voted_for = -1;
      ASSERT_TRUE(ExtractIntField(after_description, "voted_for", &after_voted_for))
          << after_description;

      EXPECT_EQ(after_status.term, before_status.term) << after_description;
      EXPECT_EQ(after_status.commit_index, before_status.commit_index) << after_description;
      EXPECT_EQ(after_status.last_applied, before_status.last_applied) << after_description;
      EXPECT_EQ(after_voted_for, before_voted_for) << after_description;

      std::string actual;
      EXPECT_TRUE(restarted->DebugGetValue("hard_state_alpha", &actual));
      EXPECT_EQ(actual, "1");
      EXPECT_TRUE(restarted->DebugGetValue("hard_state_beta", &actual));
      EXPECT_EQ(actual, "2");
    }

  } // namespace
} // namespace raftdemo
