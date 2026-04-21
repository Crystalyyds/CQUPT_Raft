#include <algorithm>
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

namespace raftdemo {
namespace {

struct RunningCluster {
  std::vector<std::shared_ptr<RaftNode>> nodes;
  std::vector<std::thread> threads;
};

const char* ProposeStatusName(ProposeStatus status) {
  switch (status) {
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

bool IsLeaderNode(const std::shared_ptr<RaftNode>& node) {
  return node->Describe().find("role=Leader") != std::string::npos;
}

std::shared_ptr<RaftNode> WaitForLeader(
    const std::vector<std::shared_ptr<RaftNode>>& nodes,
    std::chrono::milliseconds timeout) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    for (const auto& node : nodes) {
      if (IsLeaderNode(node)) {
        return node;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  return nullptr;
}

std::vector<NodeConfig> BuildThreeNodeConfigs(const std::string& data_root, int base_port) {
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
  n1.data_dir = data_root + "/node_1";

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
  n2.data_dir = data_root + "/node_2";

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
  n3.data_dir = data_root + "/node_3";

  return {n1, n2, n3};
}

RunningCluster StartCluster(const std::vector<NodeConfig>& configs) {
  RunningCluster cluster;
  cluster.nodes.reserve(configs.size());
  for (const auto& cfg : configs) {
    cluster.nodes.push_back(std::make_shared<RaftNode>(cfg));
  }

  cluster.threads.reserve(cluster.nodes.size());
  for (const auto& node : cluster.nodes) {
    cluster.threads.emplace_back([node]() {
      node->Start();
      node->Wait();
    });
  }
  return cluster;
}

void StopCluster(RunningCluster* cluster) {
  if (cluster == nullptr) {
    return;
  }
  for (auto& node : cluster->nodes) {
    node->Stop();
  }
  for (auto& t : cluster->threads) {
    if (t.joinable()) {
      t.join();
    }
  }
}

bool ProposeSet(const std::shared_ptr<RaftNode>& leader, const std::string& key,
                const std::string& value) {
  Command cmd;
  cmd.type = CommandType::kSet;
  cmd.key = key;
  cmd.value = value;
  const ProposeResult result = leader->Propose(cmd);
  Log("test", "propose SET ", key, "=", value, ", status=", ProposeStatusName(result.status),
      ", message=", result.message);
  return result.Ok();
}

bool WaitUntilValue(const std::vector<std::shared_ptr<RaftNode>>& nodes,
                    const std::string& key,
                    const std::string& expected_value,
                    std::chrono::milliseconds timeout) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    bool all_ok = true;
    for (const auto& node : nodes) {
      std::string actual;
      if (!node->DebugGetValue(key, &actual) || actual != expected_value) {
        all_ok = false;
        break;
      }
    }
    if (all_ok) {
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  return false;
}

bool WaitUntilMissing(const std::vector<std::shared_ptr<RaftNode>>& nodes,
                      const std::string& key,
                      std::chrono::milliseconds timeout) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    bool all_missing = true;
    for (const auto& node : nodes) {
      std::string actual;
      if (node->DebugGetValue(key, &actual)) {
        all_missing = false;
        break;
      }
    }
    if (all_missing) {
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  return false;
}

bool TestFullClusterRestartRecovery() {
  const std::string data_root = "./raft_data/test_full_restart";
  std::filesystem::remove_all(data_root);

  auto configs = BuildThreeNodeConfigs(data_root, 53050);
  auto cluster = StartCluster(configs);

  auto leader = WaitForLeader(cluster.nodes, std::chrono::milliseconds(6000));
  if (!leader) {
    Log("test", "TestFullClusterRestartRecovery: no leader elected in phase 1");
    StopCluster(&cluster);
    return false;
  }

  if (!ProposeSet(leader, "alpha", "1") || !ProposeSet(leader, "beta", "2")) {
    StopCluster(&cluster);
    return false;
  }

  if (!WaitUntilValue(cluster.nodes, "alpha", "1", std::chrono::milliseconds(5000)) ||
      !WaitUntilValue(cluster.nodes, "beta", "2", std::chrono::milliseconds(5000))) {
    Log("test", "TestFullClusterRestartRecovery: values not replicated before stop");
    StopCluster(&cluster);
    return false;
  }

  StopCluster(&cluster);
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  cluster = StartCluster(configs);
  leader = WaitForLeader(cluster.nodes, std::chrono::milliseconds(6000));
  if (!leader) {
    Log("test", "TestFullClusterRestartRecovery: no leader elected after restart");
    StopCluster(&cluster);
    return false;
  }

  const bool restored_alpha =
      WaitUntilValue(cluster.nodes, "alpha", "1", std::chrono::milliseconds(8000));
  const bool restored_beta =
      WaitUntilValue(cluster.nodes, "beta", "2", std::chrono::milliseconds(8000));

  StopCluster(&cluster);
  if (!restored_alpha || !restored_beta) {
    Log("test", "TestFullClusterRestartRecovery: persisted values were not restored");
    return false;
  }

  Log("test", "TestFullClusterRestartRecovery: passed");
  return true;
}

bool TestFollowerRestartCatchUp() {
  const std::string data_root = "./raft_data/test_follower_restart";
  std::filesystem::remove_all(data_root);

  auto configs = BuildThreeNodeConfigs(data_root, 53150);
  auto cluster = StartCluster(configs);

  auto leader = WaitForLeader(cluster.nodes, std::chrono::milliseconds(6000));
  if (!leader) {
    Log("test", "TestFollowerRestartCatchUp: no leader elected");
    StopCluster(&cluster);
    return false;
  }

  std::shared_ptr<RaftNode> follower;
  for (const auto& node : cluster.nodes) {
    if (node != leader) {
      follower = node;
      break;
    }
  }
  if (!follower) {
    StopCluster(&cluster);
    return false;
  }

  if (!ProposeSet(leader, "first", "100")) {
    StopCluster(&cluster);
    return false;
  }
  if (!WaitUntilValue(cluster.nodes, "first", "100", std::chrono::milliseconds(5000))) {
    StopCluster(&cluster);
    return false;
  }

  follower->Stop();
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  if (!ProposeSet(leader, "second", "200")) {
    StopCluster(&cluster);
    return false;
  }

  std::vector<std::shared_ptr<RaftNode>> alive_nodes;
  for (const auto& node : cluster.nodes) {
    if (node != follower) {
      alive_nodes.push_back(node);
    }
  }
  if (!WaitUntilValue(alive_nodes, "second", "200", std::chrono::milliseconds(5000))) {
    StopCluster(&cluster);
    return false;
  }

  auto follower_it = std::find(cluster.nodes.begin(), cluster.nodes.end(), follower);
  if (follower_it == cluster.nodes.end()) {
    StopCluster(&cluster);
    return false;
  }
  const std::size_t follower_index = static_cast<std::size_t>(std::distance(cluster.nodes.begin(), follower_it));
  if (cluster.threads[follower_index].joinable()) {
    cluster.threads[follower_index].join();
  }

  auto restarted_follower = std::make_shared<RaftNode>(configs[follower_index]);
  cluster.nodes[follower_index] = restarted_follower;
  cluster.threads[follower_index] = std::thread([restarted_follower]() {
    restarted_follower->Start();
    restarted_follower->Wait();
  });

  const bool caught_up_first =
      WaitUntilValue(cluster.nodes, "first", "100", std::chrono::milliseconds(8000));
  const bool caught_up_second =
      WaitUntilValue(cluster.nodes, "second", "200", std::chrono::milliseconds(8000));

  StopCluster(&cluster);
  if (!caught_up_first || !caught_up_second) {
    Log("test", "TestFollowerRestartCatchUp: restarted follower did not catch up");
    return false;
  }

  Log("test", "TestFollowerRestartCatchUp: passed");
  return true;
}

}  // namespace
}  // namespace raftdemo

int main() {
  using namespace raftdemo;

  const bool test1 = TestFullClusterRestartRecovery();
  const bool test2 = TestFollowerRestartCatchUp();

  if (!test1 || !test2) {
    Log("test", "persistence tests failed");
    return 1;
  }

  Log("test", "all persistence tests passed");
  return 0;
}
