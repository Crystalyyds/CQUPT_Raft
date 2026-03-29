#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "raft/config.h"
#include "raft/logging.h"
#include "raft/raft_node.h"

int main() {
  using namespace std::chrono_literals;
  using raftdemo::NodeConfig;
  using raftdemo::PeerConfig;
  using raftdemo::RaftNode;

  std::vector<NodeConfig> configs = {
      NodeConfig{.node_id = 1,
                 .address = "127.0.0.1:50051",
                 .peers = {{2, "127.0.0.1:50052"}, {3, "127.0.0.1:50053"}},
                 .election_timeout_min = 350ms,
                 .election_timeout_max = 700ms,
                 .heartbeat_interval = 100ms,
                 .rpc_deadline = 200ms},
      NodeConfig{.node_id = 2,
                 .address = "127.0.0.1:50052",
                 .peers = {{1, "127.0.0.1:50051"}, {3, "127.0.0.1:50053"}},
                 .election_timeout_min = 350ms,
                 .election_timeout_max = 700ms,
                 .heartbeat_interval = 100ms,
                 .rpc_deadline = 200ms},
      NodeConfig{.node_id = 3,
                 .address = "127.0.0.1:50053",
                 .peers = {{1, "127.0.0.1:50051"}, {2, "127.0.0.1:50052"}},
                 .election_timeout_min = 350ms,
                 .election_timeout_max = 700ms,
                 .heartbeat_interval = 100ms,
                 .rpc_deadline = 200ms},
  };

  std::vector<std::shared_ptr<RaftNode>> nodes;
  nodes.reserve(configs.size());
  for (auto& cfg : configs) {
    nodes.push_back(std::make_shared<RaftNode>(cfg));
  }

  std::vector<std::jthread> host_threads;
  host_threads.reserve(nodes.size());

  for (const auto& node : nodes) {
    host_threads.emplace_back([node] {
      node->Start();
      node->Wait();
    });
  }

  std::this_thread::sleep_for(5s);
  raftdemo::Log("main", "cluster snapshot after 5s:");
  for (const auto& node : nodes) {
    raftdemo::Log("main", node->Describe());
  }

  std::this_thread::sleep_for(5s);
  raftdemo::Log("main", "stop all nodes");
  for (const auto& node : nodes) {
    node->Stop();
  }

  return 0;
}
