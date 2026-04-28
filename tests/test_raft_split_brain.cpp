#include <gtest/gtest.h>

#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "raft/command.h"
#include "raft/config.h"
#include "raft/propose.h"
#include "raft/raft_node.h"

namespace raftdemo {
namespace {

using namespace std::chrono_literals;
namespace fs = std::filesystem;

fs::path TestBinaryDir() {
#ifdef RAFT_TEST_BINARY_DIR
  return fs::path(RAFT_TEST_BINARY_DIR);
#else
  return fs::current_path();
#endif
}

std::uint64_t NowForPath() {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
}

std::string SafeTestName() {
  const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
  std::string name = std::string(info->test_suite_name()) + "." + info->name();
  for (char& ch : name) {
    if (ch == '/' || ch == '\\' || ch == ':' || ch == ' ') {
      ch = '_';
    }
  }
  return name;
}

fs::path MakeTestRoot() {
  std::random_device rd;
  return TestBinaryDir() / "raft_test_data" / "split_brain" /
         (SafeTestName() + "_" + std::to_string(NowForPath()) + "_" +
          std::to_string(rd()));
}

int PickBasePort() {
  if (const char* env = std::getenv("RAFT_TEST_BASE_PORT")) {
    try {
      return std::stoi(env);
    } catch (...) {
    }
  }

  std::random_device rd;
  return 43000 + static_cast<int>(rd() % 12000);
}

bool Contains(const std::string& text, const std::string& needle) {
  return text.find(needle) != std::string::npos;
}

bool IsLeaderSnapshot(const std::string& snapshot) {
  return Contains(snapshot, "role=Leader");
}

std::optional<std::uint64_t> ParseUintField(const std::string& text,
                                            const std::string& field) {
  const auto pos = text.find(field);
  if (pos == std::string::npos) {
    return std::nullopt;
  }

  std::size_t begin = pos + field.size();
  std::size_t end = begin;
  while (end < text.size() && text[end] >= '0' && text[end] <= '9') {
    ++end;
  }
  if (end == begin) {
    return std::nullopt;
  }

  try {
    return static_cast<std::uint64_t>(std::stoull(text.substr(begin, end - begin)));
  } catch (...) {
    return std::nullopt;
  }
}

Command SetCommand(const std::string& key, const std::string& value) {
  Command command;
  command.type = CommandType::kSet;
  command.key = key;
  command.value = value;
  return command;
}

std::vector<NodeConfig> BuildThreeNodeConfigs(int base_port, const fs::path& root) {
  std::vector<NodeConfig> configs;
  configs.reserve(3);

  for (int id = 1; id <= 3; ++id) {
    NodeConfig cfg;
    cfg.node_id = id;
    cfg.address = "127.0.0.1:" + std::to_string(base_port + id);
    cfg.election_timeout_min = 250ms;
    cfg.election_timeout_max = 500ms;
    cfg.heartbeat_interval = 60ms;
    cfg.rpc_deadline = 50ms;
    cfg.data_dir = (root / "raft_data" / ("node_" + std::to_string(id))).string();

    for (int peer_id = 1; peer_id <= 3; ++peer_id) {
      if (peer_id == id) {
        continue;
      }
      cfg.peers.push_back(PeerConfig{
          peer_id,
          "127.0.0.1:" + std::to_string(base_port + peer_id),
      });
    }

    configs.push_back(std::move(cfg));
  }

  return configs;
}

NodeConfig BuildSingleNodeConfig(const fs::path& root) {
  NodeConfig cfg;
  cfg.node_id = 1;
  cfg.address = "127.0.0.1:0";
  cfg.election_timeout_min = 1h;
  cfg.election_timeout_max = 1h;
  cfg.heartbeat_interval = 1h;
  cfg.rpc_deadline = 50ms;
  cfg.data_dir = (root / "raft_data" / "node_1").string();
  return cfg;
}

snapshotConfig DisabledSnapshotConfig(const fs::path& root) {
  snapshotConfig cfg;
  cfg.enabled = false;
  cfg.snapshot_dir = (root / "raft_snapshots").string();
  return cfg;
}

class SplitBrainCluster {
 public:
  explicit SplitBrainCluster(int base_port) : root_(MakeTestRoot()) {
    std::error_code ec;
    fs::remove_all(root_, ec);
    fs::create_directories(root_, ec);

    snapshot_config_ = DisabledSnapshotConfig(root_);

    const auto configs = BuildThreeNodeConfigs(base_port, root_);
    nodes_.reserve(configs.size());
    for (const auto& cfg : configs) {
      NodeRuntime runtime;
      runtime.node_id = cfg.node_id;
      runtime.node = std::make_shared<RaftNode>(cfg, snapshot_config_);
      nodes_.push_back(std::move(runtime));
    }
  }

  ~SplitBrainCluster() { StopAll(); }

  void StartAll() {
    for (std::size_t i = 0; i < nodes_.size(); ++i) {
      StartNode(i);
    }
  }

  void StopAll() {
    for (std::size_t i = 0; i < nodes_.size(); ++i) {
      StopNode(i);
    }
  }

  void StartNode(std::size_t index) {
    auto& runtime = nodes_.at(index);
    if (runtime.running) {
      return;
    }

    runtime.running = true;
    auto node = runtime.node;
    runtime.thread = std::thread([node]() {
      node->Start();
      node->Wait();
    });
  }

  void StopNode(std::size_t index) {
    auto& runtime = nodes_.at(index);
    if (!runtime.running) {
      return;
    }

    runtime.node->Stop();
    if (runtime.thread.joinable()) {
      runtime.thread.join();
    }
    runtime.running = false;
  }

  std::optional<std::size_t> WaitForLeader(std::chrono::milliseconds timeout) const {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
      for (std::size_t i = 0; i < nodes_.size(); ++i) {
        if (!nodes_[i].running) {
          continue;
        }
        if (IsLeaderSnapshot(nodes_[i].node->Describe())) {
          return i;
        }
      }
      std::this_thread::sleep_for(50ms);
    }
    return std::nullopt;
  }

  bool WaitUntilValueOnAllRunning(const std::string& key, const std::string& value,
                                  std::chrono::milliseconds timeout) const {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
      bool ok = true;
      for (const auto& runtime : nodes_) {
        if (!runtime.running) {
          continue;
        }
        std::string actual;
        if (!runtime.node->DebugGetValue(key, &actual) || actual != value) {
          ok = false;
          break;
        }
      }
      if (ok) {
        return true;
      }
      std::this_thread::sleep_for(50ms);
    }
    return false;
  }

  std::shared_ptr<RaftNode> Node(std::size_t index) const {
    return nodes_.at(index).node;
  }

  int NodeId(std::size_t index) const {
    return nodes_.at(index).node_id;
  }

  std::vector<std::size_t> OtherIndexes(std::size_t index) const {
    std::vector<std::size_t> result;
    for (std::size_t i = 0; i < nodes_.size(); ++i) {
      if (i != index) {
        result.push_back(i);
      }
    }
    return result;
  }

 private:
  struct NodeRuntime {
    int node_id{0};
    std::shared_ptr<RaftNode> node;
    std::thread thread;
    bool running{false};
  };

  fs::path root_;
  snapshotConfig snapshot_config_;
  std::vector<NodeRuntime> nodes_;
};

TEST(RaftSplitBrainTest, MinorityLeaderTimesOutAndDoesNotApplyUncommittedCommand) {
  SplitBrainCluster cluster(PickBasePort());
  cluster.StartAll();

  const auto leader_index = cluster.WaitForLeader(5s);
  ASSERT_TRUE(leader_index.has_value());

  for (const auto follower_index : cluster.OtherIndexes(*leader_index)) {
    cluster.StopNode(follower_index);
  }

  const ProposeResult result =
      cluster.Node(*leader_index)->Propose(SetCommand("minority_key", "uncommitted"));

  EXPECT_EQ(result.status, ProposeStatus::kTimeout) << result.message;
  EXPECT_GT(result.log_index, 0u);

  std::string value;
  EXPECT_FALSE(cluster.Node(*leader_index)->DebugGetValue("minority_key", &value));
}

TEST(RaftSplitBrainTest, LeaderStepsDownWhenHigherTermAppendEntriesArrives) {
  SplitBrainCluster cluster(PickBasePort());
  cluster.StartAll();

  const auto leader_index = cluster.WaitForLeader(5s);
  ASSERT_TRUE(leader_index.has_value());

  const std::string before = cluster.Node(*leader_index)->Describe();
  const auto old_term = ParseUintField(before, "term=");
  ASSERT_TRUE(old_term.has_value()) << before;

  const auto other_indexes = cluster.OtherIndexes(*leader_index);
  ASSERT_FALSE(other_indexes.empty());

  raft::AppendEntriesRequest request;
  request.set_term(*old_term + 1);
  request.set_leader_id(cluster.NodeId(other_indexes.front()));
  request.set_prev_log_index(0);
  request.set_prev_log_term(0);
  request.set_leader_commit(0);

  raft::AppendEntriesResponse response;
  cluster.Node(*leader_index)->OnAppendEntries(request, &response);

  EXPECT_TRUE(response.success());
  EXPECT_EQ(response.term(), *old_term + 1);

  const std::string after = cluster.Node(*leader_index)->Describe();
  EXPECT_TRUE(Contains(after, "role=Follower")) << after;
  EXPECT_TRUE(Contains(after, "term=" + std::to_string(*old_term + 1))) << after;

  const ProposeResult result =
      cluster.Node(*leader_index)->Propose(SetCommand("old_leader_key", "must_reject"));
  EXPECT_EQ(result.status, ProposeStatus::kNotLeader) << result.message;
}

TEST(RaftSplitBrainTest, StaleAppendEntriesIsRejectedAfterNodeObservesHigherTerm) {
  const fs::path root = MakeTestRoot();
  std::error_code ec;
  fs::remove_all(root, ec);
  fs::create_directories(root, ec);

  RaftNode node(BuildSingleNodeConfig(root), DisabledSnapshotConfig(root));

  raft::VoteRequest higher_term_vote;
  higher_term_vote.set_term(5);
  higher_term_vote.set_candidate_id(10);
  higher_term_vote.set_last_log_index(0);
  higher_term_vote.set_last_log_term(0);

  raft::VoteResponse vote_response;
  node.OnRequestVote(higher_term_vote, &vote_response);
  ASSERT_TRUE(vote_response.vote_granted());
  ASSERT_EQ(vote_response.term(), 5u);

  raft::AppendEntriesRequest stale_append;
  stale_append.set_term(4);
  stale_append.set_leader_id(9);
  stale_append.set_prev_log_index(0);
  stale_append.set_prev_log_term(0);
  stale_append.set_leader_commit(0);

  raft::AppendEntriesResponse append_response;
  node.OnAppendEntries(stale_append, &append_response);

  EXPECT_FALSE(append_response.success());
  EXPECT_EQ(append_response.term(), 5u);
}

TEST(RaftSplitBrainTest, SameTermSecondCandidateVoteIsRejected) {
  const fs::path root = MakeTestRoot();
  std::error_code ec;
  fs::remove_all(root, ec);
  fs::create_directories(root, ec);

  RaftNode node(BuildSingleNodeConfig(root), DisabledSnapshotConfig(root));

  raft::VoteRequest first;
  first.set_term(7);
  first.set_candidate_id(101);
  first.set_last_log_index(0);
  first.set_last_log_term(0);

  raft::VoteResponse first_response;
  node.OnRequestVote(first, &first_response);
  ASSERT_TRUE(first_response.vote_granted());
  ASSERT_EQ(first_response.term(), 7u);

  raft::VoteRequest second;
  second.set_term(7);
  second.set_candidate_id(102);
  second.set_last_log_index(0);
  second.set_last_log_term(0);

  raft::VoteResponse second_response;
  node.OnRequestVote(second, &second_response);
  EXPECT_FALSE(second_response.vote_granted());
  EXPECT_EQ(second_response.term(), 7u);
}

TEST(RaftSplitBrainTest, StaleCandidateVoteRequestIsRejectedEvenWithHigherTerm) {
  SplitBrainCluster cluster(PickBasePort());
  cluster.StartAll();

  const auto leader_index = cluster.WaitForLeader(5s);
  ASSERT_TRUE(leader_index.has_value());

  const ProposeResult committed =
      cluster.Node(*leader_index)->Propose(SetCommand("committed_key", "value"));
  ASSERT_EQ(committed.status, ProposeStatus::kOk) << committed.message;
  ASSERT_TRUE(cluster.WaitUntilValueOnAllRunning("committed_key", "value", 5s));

  const auto follower_indexes = cluster.OtherIndexes(*leader_index);
  ASSERT_FALSE(follower_indexes.empty());
  auto follower = cluster.Node(follower_indexes.front());

  const std::string follower_state = follower->Describe();
  const auto follower_term = ParseUintField(follower_state, "term=");
  ASSERT_TRUE(follower_term.has_value()) << follower_state;

  raft::VoteRequest request;
  request.set_term(*follower_term + 1);
  request.set_candidate_id(99);
  request.set_last_log_index(0);
  request.set_last_log_term(0);

  raft::VoteResponse response;
  follower->OnRequestVote(request, &response);

  EXPECT_EQ(response.term(), *follower_term + 1);
  EXPECT_FALSE(response.vote_granted());
}

}  // namespace
}  // namespace raftdemo
