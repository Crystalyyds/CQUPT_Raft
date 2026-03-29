#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>

namespace raftdemo {

class TimerScheduler {
 public:
  using TaskId = std::uint64_t;
  using Clock = std::chrono::steady_clock;
  using TimePoint = Clock::time_point;
  using Duration = Clock::duration;

  TimerScheduler();
  ~TimerScheduler();

  void Start();
  void Stop();

  TaskId ScheduleAfter(Duration delay, std::function<void()> fn);
  bool Cancel(TaskId id);

 private:
  struct Task {
    TimePoint deadline;
    TaskId id;
    std::function<void()> fn;
    std::atomic<bool> cancelled{false};
  };

  struct TaskCompare {
    bool operator()(const std::shared_ptr<Task>& lhs,
                    const std::shared_ptr<Task>& rhs) const {
      if (lhs->deadline == rhs->deadline) {
        return lhs->id > rhs->id;
      }
      return lhs->deadline > rhs->deadline;
    }
  };

  void Run();

  std::atomic<bool> started_{false};
  std::atomic<bool> stop_{false};
  std::atomic<TaskId> next_id_{1};

  std::mutex mu_;
  std::condition_variable cv_;
  std::priority_queue<std::shared_ptr<Task>, std::vector<std::shared_ptr<Task>>, TaskCompare>
      heap_;
  std::unordered_map<TaskId, std::shared_ptr<Task>> tasks_;
  std::thread worker_;
};

}  // namespace raftdemo
