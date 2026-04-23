#include "raft/snapshot_storage.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

namespace raftdemo {
namespace {

constexpr std::uint32_t kMetaMagic = 0x53504D31U;  // "SPM1"
constexpr std::uint32_t kMetaVersion = 1U;

template <typename T>
bool WritePod(std::ofstream& out, const T& value, std::string* error) {
  static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
  out.write(reinterpret_cast<const char*>(&value), sizeof(T));
  if (!out) {
    if (error != nullptr) {
      *error = "write binary field failed";
    }
    return false;
  }
  return true;
}

template <typename T>
bool ReadPod(std::ifstream& in, T* value, std::string* error) {
  static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
  if (value == nullptr) {
    if (error != nullptr) {
      *error = "ReadPod target must not be null";
    }
    return false;
  }
  in.read(reinterpret_cast<char*>(value), sizeof(T));
  if (!in) {
    if (error != nullptr) {
      *error = in.eof() ? "unexpected EOF" : "read binary field failed";
    }
    return false;
  }
  return true;
}

class FileSnapshotStorage final : public ISnapshotStorage {
 public:
  FileSnapshotStorage(std::string snapshot_dir, std::string file_prefix)
      : snapshot_dir_(std::move(snapshot_dir)),
        file_prefix_(std::move(file_prefix)) {}

  bool SaveSnapshotFile(const std::string& temp_snapshot_file,
                        std::uint64_t last_included_index,
                        std::uint64_t last_included_term,
                        SnapshotMeta* out_meta,
                        std::string* error) override {
    if (temp_snapshot_file.empty()) {
      if (error != nullptr) {
        *error = "temp snapshot file path is empty";
      }
      return false;
    }

    std::error_code ec;
    std::filesystem::create_directories(snapshot_dir_, ec);
    if (ec) {
      if (error != nullptr) {
        *error = "create snapshot dir failed: " + ec.message();
      }
      return false;
    }

    const auto now_ms = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    const auto ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &ts);
#else
    localtime_r(&ts, &tm);
#endif
    char time_buf[32];
    std::strftime(time_buf, sizeof(time_buf), "%Y%m%d_%H%M%S", &tm);

    const std::string base_name = file_prefix_ + "_" + std::string(time_buf) +
                                  "_idx_" + std::to_string(last_included_index) +
                                  "_term_" + std::to_string(last_included_term);
    const std::filesystem::path dir(snapshot_dir_);
    const std::filesystem::path snapshot_path = dir / (base_name + ".bin");
    const std::filesystem::path meta_path = dir / (base_name + ".meta");

    ec.clear();
    std::filesystem::copy_file(temp_snapshot_file, snapshot_path,
                               std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
      if (error != nullptr) {
        *error = "copy snapshot file failed: " + ec.message();
      }
      return false;
    }

    {
      std::ofstream out(meta_path, std::ios::binary | std::ios::trunc);
      if (!out.is_open()) {
        if (error != nullptr) {
          *error = "open snapshot meta file failed: " + meta_path.string();
        }
        return false;
      }

      const std::uint32_t magic = kMetaMagic;
      const std::uint32_t version = kMetaVersion;
      if (!WritePod(out, magic, error) ||
          !WritePod(out, version, error) ||
          !WritePod(out, now_ms, error) ||
          !WritePod(out, last_included_index, error) ||
          !WritePod(out, last_included_term, error)) {
        return false;
      }
      out.flush();
      if (!out) {
        if (error != nullptr) {
          *error = "flush snapshot meta file failed";
        }
        return false;
      }
    }

    ec.clear();
    std::filesystem::remove(temp_snapshot_file, ec);

    if (out_meta != nullptr) {
      out_meta->snapshot_path = snapshot_path.string();
      out_meta->meta_path = meta_path.string();
      out_meta->last_included_index = last_included_index;
      out_meta->last_included_term = last_included_term;
      out_meta->created_unix_ms = now_ms;
    }
    return true;
  }

  bool ListSnapshots(std::vector<SnapshotMeta>* out,
                     std::string* error) override {
    if (out == nullptr) {
      if (error != nullptr) {
        *error = "output snapshot list must not be null";
      }
      return false;
    }
    out->clear();

    std::error_code ec;
    if (!std::filesystem::exists(snapshot_dir_, ec)) {
      return true;
    }

    for (const auto& entry : std::filesystem::directory_iterator(snapshot_dir_, ec)) {
      if (ec) {
        if (error != nullptr) {
          *error = "iterate snapshot dir failed: " + ec.message();
        }
        return false;
      }
      if (!entry.is_regular_file()) {
        continue;
      }
      if (entry.path().extension() != ".meta") {
        continue;
      }

      SnapshotMeta meta;
      if (!ReadMeta(entry.path(), &meta, error)) {
        // 对损坏 meta：跳过，让加载逻辑继续尝试更旧快照。
        if (error != nullptr) {
          *error = "read snapshot meta failed: " + entry.path().string() + ", " + *error;
        }
        continue;
      }
      if (!std::filesystem::exists(meta.snapshot_path)) {
        continue;
      }
      out->push_back(std::move(meta));
    }

    std::sort(out->begin(), out->end(), [](const SnapshotMeta& a, const SnapshotMeta& b) {
      if (a.created_unix_ms != b.created_unix_ms) {
        return a.created_unix_ms > b.created_unix_ms;
      }
      return a.last_included_index > b.last_included_index;
    });
    return true;
  }

  bool LoadLatestValidSnapshot(SnapshotMeta* out_meta,
                               bool* has_snapshot,
                               std::string* error) override {
    if (has_snapshot == nullptr) {
      if (error != nullptr) {
        *error = "has_snapshot must not be null";
      }
      return false;
    }
    *has_snapshot = false;
    std::vector<SnapshotMeta> snapshots;
    if (!ListSnapshots(&snapshots, error)) {
      return false;
    }
    if (snapshots.empty()) {
      return true;
    }
    if (out_meta != nullptr) {
      *out_meta = snapshots.front();
    }
    *has_snapshot = true;
    return true;
  }

  bool PruneSnapshots(std::size_t max_keep,
                      std::string* error) override {
    if (max_keep == 0) {
      max_keep = 1;
    }
    std::vector<SnapshotMeta> snapshots;
    if (!ListSnapshots(&snapshots, error)) {
      return false;
    }
    if (snapshots.size() <= max_keep) {
      return true;
    }

    std::error_code ec;
    for (std::size_t i = max_keep; i < snapshots.size(); ++i) {
      ec.clear();
      std::filesystem::remove(snapshots[i].snapshot_path, ec);
      if (ec && error != nullptr) {
        *error = "remove old snapshot file failed: " + ec.message();
      }
      ec.clear();
      std::filesystem::remove(snapshots[i].meta_path, ec);
      if (ec && error != nullptr) {
        *error = "remove old snapshot meta failed: " + ec.message();
      }
    }
    return true;
  }

  const std::string& SnapshotDir() const override {
    return snapshot_dir_;
  }

 private:
  bool ReadMeta(const std::filesystem::path& meta_path,
                SnapshotMeta* out_meta,
                std::string* error) {
    if (out_meta == nullptr) {
      if (error != nullptr) {
        *error = "snapshot meta output must not be null";
      }
      return false;
    }

    std::ifstream in(meta_path, std::ios::binary);
    if (!in.is_open()) {
      if (error != nullptr) {
        *error = "open snapshot meta file failed";
      }
      return false;
    }

    std::uint32_t magic = 0;
    std::uint32_t version = 0;
    if (!ReadPod(in, &magic, error) ||
        !ReadPod(in, &version, error) ||
        !ReadPod(in, &out_meta->created_unix_ms, error) ||
        !ReadPod(in, &out_meta->last_included_index, error) ||
        !ReadPod(in, &out_meta->last_included_term, error)) {
      return false;
    }

    if (magic != kMetaMagic) {
      if (error != nullptr) {
        *error = "invalid snapshot meta magic";
      }
      return false;
    }
    if (version != kMetaVersion) {
      if (error != nullptr) {
        *error = "unsupported snapshot meta version";
      }
      return false;
    }

    out_meta->meta_path = meta_path.string();
    out_meta->snapshot_path = meta_path.parent_path() / (meta_path.stem().string() + ".bin");
    out_meta->snapshot_path = std::filesystem::path(out_meta->snapshot_path).string();
    return true;
  }

  std::string snapshot_dir_;
  std::string file_prefix_;
};

}  // namespace

std::unique_ptr<ISnapshotStorage> CreateFileSnapshotStorage(std::string snapshot_dir,
                                                            std::string file_prefix) {
  return std::make_unique<FileSnapshotStorage>(std::move(snapshot_dir), std::move(file_prefix));
}

}  // namespace raftdemo
