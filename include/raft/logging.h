#pragma once

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>

namespace raftdemo {

inline std::mutex& GlobalLogMutex() {
  static std::mutex mu;
  return mu;
}

inline std::string NowString() {
  using namespace std::chrono;
  const auto now = system_clock::now();
  const auto tt = system_clock::to_time_t(now);
  const auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

  std::tm tm{};
#if defined(_WIN32)
  localtime_s(&tm, &tt);
#else
  localtime_r(&tt, &tm);
#endif

  std::ostringstream oss;
  oss << std::put_time(&tm, "%H:%M:%S") << '.' << std::setw(3) << std::setfill('0')
      << ms.count();
  return oss.str();
}

template <typename... Args>
void Log(const std::string& tag, Args&&... args) {
  std::ostringstream oss;
  (oss << ... << std::forward<Args>(args));

  std::lock_guard<std::mutex> lk(GlobalLogMutex());
  std::cout << '[' << NowString() << "] [" << tag << "] " << oss.str() << std::endl;
}

}  // namespace raftdemo
