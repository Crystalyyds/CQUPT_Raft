#include "raft/raft_node.h"

#include <algorithm>
#include <chrono>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "raft/logging.h"
#include "raft/raft_service_impl.h"

namespace raftdemo {
namespace {

std::string NodeTag(int node_id) { return "node-" + std::to_string(node_id); }

}  // namespace

RaftNode::RaftNode(NodeConfig config)
    : config_(std::move(config)), rng_(std::random_device{}()) {
  log_.push_back(LogRecord{0, 0, "bootstrap"});
}

RaftNode::~RaftNode() { Stop(); }

void RaftNode::Start() {
  bool expected = false;
  if (!running_.compare_exchange_strong(expected, true)) {
    return;
  }

  InitClients();
  scheduler_.Start();
  InitServer();

  {
    std::lock_guard<std::mutex> lk(mu_);
    ResetElectionTimerLocked();
  }

  Log(NodeTag(config_.node_id), "started at ", config_.address, ", peers=", config_.peers.size());
}

void RaftNode::Stop() {
  bool expected = true;
  if (!running_.compare_exchange_strong(expected, false)) {
    return;
  }

  {
    std::lock_guard<std::mutex> lk(mu_);
    if (election_timer_id_) {
      scheduler_.Cancel(*election_timer_id_);
      election_timer_id_.reset();
    }
    if (heartbeat_timer_id_) {
      scheduler_.Cancel(*heartbeat_timer_id_);
      heartbeat_timer_id_.reset();
    }
  }

  if (server_) {
    server_->Shutdown();
  }
  scheduler_.Stop();
  rpc_pool_.Stop();

  Log(NodeTag(config_.node_id), "stopped");
}

void RaftNode::Wait() {
  if (server_) {
    server_->Wait();
  }
}

void RaftNode::InitServer() {
  service_ = std::make_unique<RaftServiceImpl>(*this);

  grpc::ServerBuilder builder;
  builder.AddListeningPort(config_.address, grpc::InsecureServerCredentials());
  builder.RegisterService(service_.get());
  server_ = builder.BuildAndStart();
  if (!server_) {
    running_.store(false);
    throw std::runtime_error("failed to start gRPC server at " + config_.address);
  }
}

void RaftNode::InitClients() {
  for (const auto& peer : config_.peers) {
    auto client = std::make_unique<PeerClient>();
    client->channel = grpc::CreateChannel(peer.address, grpc::InsecureChannelCredentials());
    client->stub = raft::RaftService::NewStub(client->channel);
    clients_[peer.node_id] = std::move(client);
  }
}

std::string RaftNode::Describe() const {
  std::lock_guard<std::mutex> lk(mu_);
  std::ostringstream oss;
  oss << "node=" << config_.node_id << ", role=" << RoleName(role_)
      << ", term=" << current_term_ << ", voted_for=" << voted_for_
      << ", leader=" << leader_id_ << ", last_log_index=" << LastLogIndexLocked()
      << ", commit_index=" << commit_index_;
  return oss.str();
}

void RaftNode::ResetElectionTimerLocked() {
  if (election_timer_id_) {
    scheduler_.Cancel(*election_timer_id_);
    election_timer_id_.reset();
  }

  const auto timeout = RandomElectionTimeoutLocked();
  auto weak = weak_from_this();
  election_timer_id_ = scheduler_.ScheduleAfter(timeout, [weak] {
    if (auto self = weak.lock()) {
      self->OnElectionTimeout();
    }
  });
}

void RaftNode::ResetHeartbeatTimerLocked() {
  if (heartbeat_timer_id_) {
    scheduler_.Cancel(*heartbeat_timer_id_);
    heartbeat_timer_id_.reset();
  }

  if (role_ != Role::kLeader) {
    return;
  }

  auto weak = weak_from_this();
  const auto interval = config_.heartbeat_interval;
  heartbeat_timer_id_ = scheduler_.ScheduleAfter(interval, [weak] {
    if (auto self = weak.lock()) {
      self->SendHeartbeats();
      std::lock_guard<std::mutex> lk(self->mu_);
      if (self->running_.load() && self->role_ == Role::kLeader) {
        self->ResetHeartbeatTimerLocked();
      }
    }
  });
}

std::chrono::milliseconds RaftNode::RandomElectionTimeoutLocked() {
  const auto min_ms = static_cast<int>(config_.election_timeout_min.count());
  const auto max_ms = static_cast<int>(config_.election_timeout_max.count());
  std::uniform_int_distribution<int> dist(min_ms, max_ms);
  return std::chrono::milliseconds(dist(rng_));
}

void RaftNode::OnElectionTimeout() {
  if (!running_.load()) {
    return;
  }

  {
    std::lock_guard<std::mutex> lk(mu_);
    if (role_ == Role::kLeader) {
      return;
    }
  }

  StartElection();
}

void RaftNode::StartElection() {
  std::uint64_t term = 0;
  std::uint64_t last_log_index = 0;
  std::uint64_t last_log_term = 0;
  std::vector<PeerConfig> peers;
  int quorum = 0;

  {
    std::lock_guard<std::mutex> lk(mu_);
    if (!running_.load() || role_ == Role::kLeader) {
      return;
    }

    role_ = Role::kCandidate;
    ++current_term_;
    voted_for_ = config_.node_id;
    leader_id_ = -1;

    term = current_term_;
    last_log_index = LastLogIndexLocked();
    last_log_term = LastLogTermLocked();
    peers = config_.peers;
    quorum = static_cast<int>((peers.size() + 1) / 2) + 1;

    ResetElectionTimerLocked();
    Log(NodeTag(config_.node_id), "start election, term=", term,
        ", last_log_index=", last_log_index, ", last_log_term=", last_log_term);
  }

  auto votes = std::make_shared<std::atomic<int>>(1);
  auto won = std::make_shared<std::atomic<bool>>(false);
  auto weak = weak_from_this();

  for (const auto& peer : peers) {
    rpc_pool_.Submit([weak, peer, term, last_log_index, last_log_term, votes, won, quorum] {
      auto self = weak.lock();
      if (!self || !self->running_.load()) {
        return;
      }

      raft::VoteRequest request;
      request.set_term(term);
      request.set_candidate_id(self->config_.node_id);
      request.set_last_log_index(last_log_index);
      request.set_last_log_term(last_log_term);

      auto response = self->RequestVoteRpc(peer.node_id, request);
      if (!response.has_value()) {
        return;
      }

      {
        std::lock_guard<std::mutex> lk(self->mu_);
        if (!self->running_.load()) {
          return;
        }
        if (response->term() > self->current_term_) {
          self->BecomeFollowerLocked(response->term(), -1,
                                     "peer replied higher term in RequestVote");
          return;
        }
        if (self->role_ != Role::kCandidate || self->current_term_ != term) {
          return;
        }
      }

      if (!response->vote_granted()) {
        return;
      }

      const int total = votes->fetch_add(1) + 1;
      if (total >= quorum && !won->exchange(true)) {
        self->OnElectionWon(term);
      }
    });
  }
}

void RaftNode::OnElectionWon(std::uint64_t term) {
  bool should_send_heartbeat = false;
  {
    std::lock_guard<std::mutex> lk(mu_);
    if (!running_.load()) {
      return;
    }
    if (role_ != Role::kCandidate || current_term_ != term) {
      return;
    }

    BecomeLeaderLocked();
    should_send_heartbeat = true;
    Log(NodeTag(config_.node_id), "won election, become leader, term=", current_term_);
  }

  if (should_send_heartbeat) {
    SendHeartbeats();
  }
}

void RaftNode::SendHeartbeats() {
  std::vector<PeerConfig> peers;
  std::uint64_t term = 0;
  std::uint64_t leader_commit = 0;

  {
    std::lock_guard<std::mutex> lk(mu_);
    if (!running_.load() || role_ != Role::kLeader) {
      return;
    }
    peers = config_.peers;
    term = current_term_;
    leader_commit = commit_index_;
  }

  auto weak = weak_from_this();
  for (const auto& peer : peers) {
    rpc_pool_.Submit([weak, peer, term, leader_commit] {
      auto self = weak.lock();
      if (!self || !self->running_.load()) {
        return;
      }

      raft::AppendEntriesRequest request;
      request.set_term(term);
      request.set_leader_id(self->config_.node_id);
      request.set_leader_commit(leader_commit);

      {
        std::lock_guard<std::mutex> lk(self->mu_);
        if (self->role_ != Role::kLeader || self->current_term_ != term) {
          return;
        }
        const auto next_index_it = self->next_index_.find(peer.node_id);
        const std::uint64_t next_index =
            next_index_it == self->next_index_.end() ? self->LastLogIndexLocked() + 1
                                                     : next_index_it->second;
        const std::uint64_t prev_log_index = next_index == 0 ? 0 : next_index - 1;
        const std::uint64_t prev_log_term =
            prev_log_index < self->log_.size() ? self->log_[prev_log_index].term : 0;

        request.set_prev_log_index(prev_log_index);
        request.set_prev_log_term(prev_log_term);

        for (std::uint64_t i = next_index; i < self->log_.size(); ++i) {
          auto* entry = request.add_entries();
          entry->set_index(self->log_[i].index);
          entry->set_term(self->log_[i].term);
          entry->set_command(self->log_[i].command);
        }
      }

      auto response = self->AppendEntriesRpc(peer.node_id, request);
      if (!response.has_value()) {
        return;
      }

      std::lock_guard<std::mutex> lk(self->mu_);
      if (!self->running_.load()) {
        return;
      }
      if (response->term() > self->current_term_) {
        self->BecomeFollowerLocked(response->term(), -1,
                                   "peer replied higher term in AppendEntries");
        return;
      }
      if (self->role_ != Role::kLeader || self->current_term_ != term) {
        return;
      }

      if (response->success()) {
        self->match_index_[peer.node_id] = response->match_index();
        self->next_index_[peer.node_id] = response->match_index() + 1;
      } else {
        auto& next_index = self->next_index_[peer.node_id];
        if (next_index > 1) {
          --next_index;
        }
      }
    });
  }
}

void RaftNode::BecomeFollowerLocked(std::uint64_t new_term, int new_leader,
                                    const std::string& reason) {
  const auto old_role = role_;
  const auto old_term = current_term_;

  if (new_term > current_term_) {
    current_term_ = new_term;
    voted_for_ = -1;
  }

  role_ = Role::kFollower;
  leader_id_ = new_leader;

  if (heartbeat_timer_id_) {
    scheduler_.Cancel(*heartbeat_timer_id_);
    heartbeat_timer_id_.reset();
  }

  ResetElectionTimerLocked();

  if (old_role != role_ || old_term != current_term_) {
    Log(NodeTag(config_.node_id), "become follower, term=", current_term_, ", leader=",
        leader_id_, ", reason=", reason);
  }
}

void RaftNode::BecomeLeaderLocked() {
  role_ = Role::kLeader;
  leader_id_ = config_.node_id;

  if (election_timer_id_) {
    scheduler_.Cancel(*election_timer_id_);
    election_timer_id_.reset();
  }

  const auto last_log_index = LastLogIndexLocked();
  next_index_.clear();
  match_index_.clear();
  for (const auto& peer : config_.peers) {
    next_index_[peer.node_id] = last_log_index + 1;
    match_index_[peer.node_id] = 0;
  }

  ResetHeartbeatTimerLocked();
}

bool RaftNode::IsCandidateLogUpToDateLocked(std::uint64_t last_log_index,
                                            std::uint64_t last_log_term) const {
  const auto my_last_term = LastLogTermLocked();
  if (last_log_term != my_last_term) {
    return last_log_term > my_last_term;
  }
  return last_log_index >= LastLogIndexLocked();
}

std::uint64_t RaftNode::LastLogIndexLocked() const { return log_.empty() ? 0 : log_.back().index; }

std::uint64_t RaftNode::LastLogTermLocked() const { return log_.empty() ? 0 : log_.back().term; }

std::optional<raft::VoteResponse> RaftNode::RequestVoteRpc(int peer_id,
                                                           const raft::VoteRequest& request) {
  auto it = clients_.find(peer_id);
  if (it == clients_.end()) {
    return std::nullopt;
  }

  grpc::ClientContext context;
  context.set_deadline(std::chrono::system_clock::now() + config_.rpc_deadline);

  raft::VoteResponse response;
  grpc::Status status;
  {
    std::lock_guard<std::mutex> lk(it->second->mu);
    status = it->second->stub->RequestVote(&context, request, &response);
  }

  if (!status.ok()) {
    return std::nullopt;
  }
  return response;
}

std::optional<raft::AppendEntriesResponse> RaftNode::AppendEntriesRpc(
    int peer_id, const raft::AppendEntriesRequest& request) {
  auto it = clients_.find(peer_id);
  if (it == clients_.end()) {
    return std::nullopt;
  }

  grpc::ClientContext context;
  context.set_deadline(std::chrono::system_clock::now() + config_.rpc_deadline);

  raft::AppendEntriesResponse response;
  grpc::Status status;
  {
    std::lock_guard<std::mutex> lk(it->second->mu);
    status = it->second->stub->AppendEntries(&context, request, &response);
  }

  if (!status.ok()) {
    return std::nullopt;
  }
  return response;
}

void RaftNode::OnRequestVote(const raft::VoteRequest& request, raft::VoteResponse* response) {
  std::lock_guard<std::mutex> lk(mu_);
  response->set_term(current_term_);
  response->set_vote_granted(false);

  if (request.term() < current_term_) {
    return;
  }

  if (request.term() > current_term_) {
    BecomeFollowerLocked(request.term(), -1, "received higher term vote request");
  }

  const bool up_to_date =
      IsCandidateLogUpToDateLocked(request.last_log_index(), request.last_log_term());
  const bool can_vote = (voted_for_ == -1 || voted_for_ == request.candidate_id());

  if (can_vote && up_to_date) {
    voted_for_ = request.candidate_id();
    response->set_vote_granted(true);
    ResetElectionTimerLocked();
    Log(NodeTag(config_.node_id), "grant vote to candidate=", request.candidate_id(),
        ", term=", current_term_);
  }

  response->set_term(current_term_);
}

void RaftNode::OnAppendEntries(const raft::AppendEntriesRequest& request,
                               raft::AppendEntriesResponse* response) {
  std::lock_guard<std::mutex> lk(mu_);
  response->set_term(current_term_);
  response->set_success(false);
  response->set_match_index(LastLogIndexLocked());

  if (request.term() < current_term_) {
    return;
  }

  if (request.term() > current_term_ || role_ != Role::kFollower ||
      leader_id_ != request.leader_id()) {
    BecomeFollowerLocked(request.term(), request.leader_id(), "received append entries");
  } else {
    leader_id_ = request.leader_id();
    ResetElectionTimerLocked();
  }

  if (request.prev_log_index() >= log_.size()) {
    response->set_term(current_term_);
    return;
  }

  if (log_[request.prev_log_index()].term != request.prev_log_term()) {
    response->set_term(current_term_);
    return;
  }

  std::uint64_t append_at = request.prev_log_index() + 1;
  int req_idx = 0;
  while (req_idx < request.entries_size() && append_at < log_.size()) {
    const auto& req_entry = request.entries(req_idx);
    if (log_[append_at].term != req_entry.term()) {
      log_.resize(append_at);
      break;
    }
    ++append_at;
    ++req_idx;
  }

  for (; req_idx < request.entries_size(); ++req_idx) {
    const auto& req_entry = request.entries(req_idx);
    log_.push_back(LogRecord{req_entry.index(), req_entry.term(), req_entry.command()});
  }

  if (request.leader_commit() > commit_index_) {
    commit_index_ = std::min<std::uint64_t>(request.leader_commit(), LastLogIndexLocked());
  }

  response->set_term(current_term_);
  response->set_success(true);
  response->set_match_index(LastLogIndexLocked());
}

const char* RaftNode::RoleName(Role role) {
  switch (role) {
    case Role::kFollower:
      return "Follower";
    case Role::kCandidate:
      return "Candidate";
    case Role::kLeader:
      return "Leader";
  }
  return "Unknown";
}

}  // namespace raftdemo
