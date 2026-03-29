#pragma once

#include <chrono>
#include <string>
#include <vector>

namespace raftdemo {

struct PeerConfig {
  int node_id;
  std::string address;
};

struct NodeConfig {
  int node_id;
  std::string address;
  std::vector<PeerConfig> peers;

  std::chrono::milliseconds election_timeout_min{300};
  std::chrono::milliseconds election_timeout_max{600};
  std::chrono::milliseconds heartbeat_interval{80};
  std::chrono::milliseconds rpc_deadline{250};
};

}  // namespace raftdemo
