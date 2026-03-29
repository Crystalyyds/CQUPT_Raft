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

  void Submit(std::function<void()> task);
  void Stop();

 private:
  void WorkerLoop();

  std::mutex mu_;
  std::condition_variable cv_;
  bool stop_{false};
  std::queue<std::function<void()>> tasks_;
  std::vector<std::thread> workers_;
};

}  // namespace raftdemo
