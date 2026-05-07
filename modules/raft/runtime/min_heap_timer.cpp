#include "raft/min_heap_timer.h"

namespace raftdemo {

TimerScheduler::TimerScheduler() = default;

TimerScheduler::~TimerScheduler() { Stop(); }

void TimerScheduler::Start() {
  bool expected = false;
  if (!started_.compare_exchange_strong(expected, true)) {
    return;
  }
  stop_.store(false);
  worker_ = std::thread(&TimerScheduler::Run, this);
}

void TimerScheduler::Stop() {
  if (!started_.load()) {
    return;
  }

  {
    std::lock_guard<std::mutex> lk(mu_);
    stop_.store(true);
  }
  cv_.notify_all();

  if (worker_.joinable()) {
    worker_.join();
  }

  {
    std::lock_guard<std::mutex> lk(mu_);
    while (!heap_.empty()) {
      heap_.pop();
    }
    tasks_.clear();
  }
  started_.store(false);
}

TimerScheduler::TaskId TimerScheduler::ScheduleAfter(Duration delay, std::function<void()> fn) {
  auto task = std::make_shared<Task>();
  task->deadline = Clock::now() + delay;
  task->id = next_id_.fetch_add(1);
  task->fn = std::move(fn);

  {
    std::lock_guard<std::mutex> lk(mu_);
    if (stop_.load()) {
      return 0;
    }
    tasks_[task->id] = task;
    heap_.push(task);
  }
  cv_.notify_one();
  return task->id;
}

bool TimerScheduler::Cancel(TaskId id) {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = tasks_.find(id);
  if (it == tasks_.end()) {
    return false;
  }
  it->second->cancelled.store(true);
  tasks_.erase(it);
  cv_.notify_one();
  return true;
}

void TimerScheduler::Run() {
  std::unique_lock<std::mutex> lk(mu_);

  while (!stop_.load()) {
    if (heap_.empty()) {
      cv_.wait(lk, [this] { return stop_.load() || !heap_.empty(); });
      continue;
    }

    auto task = heap_.top();
    if (task->cancelled.load()) {
      heap_.pop();
      continue;
    }

    const auto now = Clock::now();
    if (task->deadline > now) {
      cv_.wait_until(lk, task->deadline);
      continue;
    }

    heap_.pop();
    tasks_.erase(task->id);
    if (task->cancelled.load()) {
      continue;
    }

    auto fn = std::move(task->fn);
    lk.unlock();
    fn();
    lk.lock();
  }
}

}  // namespace raftdemo
