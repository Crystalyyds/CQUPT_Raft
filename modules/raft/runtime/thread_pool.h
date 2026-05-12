#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace raftdemo {

class ThreadPool {
 public:
  explicit ThreadPool(std::size_t workers);
  ~ThreadPool();

  void Start();
  void Submit(std::function<void()> task);
  void Stop();

 private:
  void WorkerLoop();

  std::size_t worker_count_{1};
  std::mutex mu_;
  std::condition_variable cv_;
  bool stop_{true};
  std::queue<std::function<void()>> tasks_;
  std::vector<std::thread> workers_;
};

}  // namespace raftdemo
