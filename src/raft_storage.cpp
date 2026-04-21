#include "raft/raft_storage.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include "raft/raft_node.h"

namespace raftdemo {
namespace {

constexpr std::uint32_t kFileMagic = 0x52465431U;   // "RFT1"
constexpr std::uint32_t kFileVersion = 1U;
constexpr const char* kStateFileName = "raft_state.bin";
constexpr const char* kTempFileName = "raft_state.bin.tmp";

std::string ErrnoMessage(const std::string& prefix) {
  return prefix + ": " + std::strerror(errno);
}

bool WriteAll(int fd, const void* data, std::size_t length, std::string* error) {
  const char* ptr = static_cast<const char*>(data);
  std::size_t written = 0;
  while (written < length) {
    const ssize_t rc = ::write(fd, ptr + written, length - written);
    if (rc < 0) {
      if (errno == EINTR) {
        continue;
      }
      if (error != nullptr) {
        *error = ErrnoMessage("write failed");
      }
      return false;
    }
    written += static_cast<std::size_t>(rc);
  }
  return true;
}

bool ReadAll(int fd, void* data, std::size_t length, std::string* error) {
  char* ptr = static_cast<char*>(data);
  std::size_t read_bytes = 0;
  while (read_bytes < length) {
    const ssize_t rc = ::read(fd, ptr + read_bytes, length - read_bytes);
    if (rc == 0) {
      if (error != nullptr) {
        *error = "unexpected EOF";
      }
      return false;
    }
    if (rc < 0) {
      if (errno == EINTR) {
        continue;
      }
      if (error != nullptr) {
        *error = ErrnoMessage("read failed");
      }
      return false;
    }
    read_bytes += static_cast<std::size_t>(rc);
  }
  return true;
}

bool FsyncDir(const std::string& dir_path, std::string* error) {
  const int dir_fd = ::open(dir_path.c_str(), O_RDONLY | O_DIRECTORY);
  if (dir_fd < 0) {
    if (error != nullptr) {
      *error = ErrnoMessage("open data dir failed");
    }
    return false;
  }

  const int rc = ::fsync(dir_fd);
  const int saved_errno = errno;
  ::close(dir_fd);
  if (rc != 0) {
    errno = saved_errno;
    if (error != nullptr) {
      *error = ErrnoMessage("fsync data dir failed");
    }
    return false;
  }
  return true;
}

class FileRaftStorage final : public IRaftStorage {
 public:
  explicit FileRaftStorage(std::string data_dir) : data_dir_(std::move(data_dir)) {}

  bool Load(PersistentRaftState* state, bool* has_state, std::string* error) override {
    if (state == nullptr || has_state == nullptr) {
      if (error != nullptr) {
        *error = "Load arguments must not be null";
      }
      return false;
    }

    *has_state = false;
    state->current_term = 0;
    state->voted_for = -1;
    state->log.clear();

    const std::filesystem::path path = std::filesystem::path(data_dir_) / kStateFileName;
    if (!std::filesystem::exists(path)) {
      return true;
    }

    const int fd = ::open(path.c_str(), O_RDONLY);
    if (fd < 0) {
      if (error != nullptr) {
        *error = ErrnoMessage("open state file failed");
      }
      return false;
    }

    auto close_fd = [&fd]() {
      if (fd >= 0) {
        ::close(fd);
      }
    };

    std::uint32_t magic = 0;
    std::uint32_t version = 0;
    std::int64_t voted_for = -1;
    std::uint64_t log_count = 0;

    if (!ReadAll(fd, &magic, sizeof(magic), error) ||
        !ReadAll(fd, &version, sizeof(version), error) ||
        !ReadAll(fd, &state->current_term, sizeof(state->current_term), error) ||
        !ReadAll(fd, &voted_for, sizeof(voted_for), error) ||
        !ReadAll(fd, &log_count, sizeof(log_count), error)) {
      close_fd();
      return false;
    }

    if (magic != kFileMagic) {
      close_fd();
      if (error != nullptr) {
        *error = "invalid raft state magic";
      }
      return false;
    }
    if (version != kFileVersion) {
      close_fd();
      if (error != nullptr) {
        *error = "unsupported raft state version";
      }
      return false;
    }

    state->voted_for = static_cast<int>(voted_for);
    state->log.reserve(static_cast<std::size_t>(log_count));

    for (std::uint64_t i = 0; i < log_count; ++i) {
      LogRecord record{};
      std::uint64_t command_size = 0;
      if (!ReadAll(fd, &record.index, sizeof(record.index), error) ||
          !ReadAll(fd, &record.term, sizeof(record.term), error) ||
          !ReadAll(fd, &command_size, sizeof(command_size), error)) {
        close_fd();
        return false;
      }
      record.command.resize(static_cast<std::size_t>(command_size));
      if (command_size > 0 &&
          !ReadAll(fd, record.command.data(), static_cast<std::size_t>(command_size), error)) {
        close_fd();
        return false;
      }
      state->log.push_back(std::move(record));
    }

    close_fd();
    *has_state = true;
    return true;
  }

  bool Save(const PersistentRaftState& state, std::string* error) override {
    std::error_code ec;
    std::filesystem::create_directories(data_dir_, ec);
    if (ec) {
      if (error != nullptr) {
        *error = "create data dir failed: " + ec.message();
      }
      return false;
    }

    const std::filesystem::path temp_path = std::filesystem::path(data_dir_) / kTempFileName;
    const std::filesystem::path state_path = std::filesystem::path(data_dir_) / kStateFileName;

    const int fd = ::open(temp_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
      if (error != nullptr) {
        *error = ErrnoMessage("open temp state file failed");
      }
      return false;
    }

    auto close_fd = [&fd]() {
      if (fd >= 0) {
        ::close(fd);
      }
    };

    const std::uint32_t magic = kFileMagic;
    const std::uint32_t version = kFileVersion;
    const std::int64_t voted_for = static_cast<std::int64_t>(state.voted_for);
    const std::uint64_t log_count = static_cast<std::uint64_t>(state.log.size());

    if (!WriteAll(fd, &magic, sizeof(magic), error) ||
        !WriteAll(fd, &version, sizeof(version), error) ||
        !WriteAll(fd, &state.current_term, sizeof(state.current_term), error) ||
        !WriteAll(fd, &voted_for, sizeof(voted_for), error) ||
        !WriteAll(fd, &log_count, sizeof(log_count), error)) {
      close_fd();
      return false;
    }

    for (const auto& record : state.log) {
      const std::uint64_t command_size = static_cast<std::uint64_t>(record.command.size());
      if (!WriteAll(fd, &record.index, sizeof(record.index), error) ||
          !WriteAll(fd, &record.term, sizeof(record.term), error) ||
          !WriteAll(fd, &command_size, sizeof(command_size), error)) {
        close_fd();
        return false;
      }
      if (command_size > 0 &&
          !WriteAll(fd, record.command.data(), static_cast<std::size_t>(command_size), error)) {
        close_fd();
        return false;
      }
    }

    if (::fsync(fd) != 0) {
      if (error != nullptr) {
        *error = ErrnoMessage("fsync temp state file failed");
      }
      close_fd();
      return false;
    }

    close_fd();

    if (::rename(temp_path.c_str(), state_path.c_str()) != 0) {
      if (error != nullptr) {
        *error = ErrnoMessage("rename state file failed");
      }
      return false;
    }

    return FsyncDir(data_dir_, error);
  }

  const std::string& DataDir() const override { return data_dir_; }

 private:
  std::string data_dir_;
};

}  // namespace

std::unique_ptr<IRaftStorage> CreateFileRaftStorage(std::string data_dir) {
  return std::make_unique<FileRaftStorage>(std::move(data_dir));
}

}  // namespace raftdemo
