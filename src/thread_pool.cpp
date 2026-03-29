#include "raft/thread_pool.h"

namespace raftdemo {

ThreadPool::ThreadPool(std::size_t workers) {
  if (workers == 0) {
    workers = 1;
  }
  workers_.reserve(workers);
  for (std::size_t i = 0; i < workers; ++i) {
    workers_.emplace_back(&ThreadPool::WorkerLoop, this);
  }
}

ThreadPool::~ThreadPool() { Stop(); }

void ThreadPool::Submit(std::function<void()> task) {
  {
    std::lock_guard<std::mutex> lk(mu_);
    if (stop_) {
      return;
    }
    tasks_.push(std::move(task));
  }
  cv_.notify_one();
}

void ThreadPool::Stop() {
  {
    std::lock_guard<std::mutex> lk(mu_);
    if (stop_) {
      return;
    }
    stop_ = true;
  }
  cv_.notify_all();

  for (auto& worker : workers_) {
    if (worker.joinable()) {
      worker.join();
    }
  }
  workers_.clear();
}

void ThreadPool::WorkerLoop() {
  while (true) {
    std::function<void()> task;
    {
      std::unique_lock<std::mutex> lk(mu_);
      cv_.wait(lk, [this] { return stop_ || !tasks_.empty(); });
      if (stop_ && tasks_.empty()) {
        return;
      }
      task = std::move(tasks_.front());
      tasks_.pop();
    }
    task();
  }
}

}  // namespace raftdemo
