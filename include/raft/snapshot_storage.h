#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace raftdemo {

struct SnapshotMeta {
  std::string snapshot_path;
  std::string meta_path;
  std::uint64_t last_included_index{0};
  std::uint64_t last_included_term{0};
  std::uint64_t created_unix_ms{0};
};

class ISnapshotStorage {
 public:
  virtual ~ISnapshotStorage() = default;

  virtual bool SaveSnapshotFile(const std::string& temp_snapshot_file,
                                std::uint64_t last_included_index,
                                std::uint64_t last_included_term,
                                SnapshotMeta* out_meta,
                                std::string* error) = 0;

  virtual bool ListSnapshots(std::vector<SnapshotMeta>* out,
                             std::string* error) = 0;

  virtual bool LoadLatestValidSnapshot(SnapshotMeta* out_meta,
                                       bool* has_snapshot,
                                       std::string* error) = 0;

  virtual bool PruneSnapshots(std::size_t max_keep,
                              std::string* error) = 0;

  virtual const std::string& SnapshotDir() const = 0;
};

std::unique_ptr<ISnapshotStorage> CreateFileSnapshotStorage(std::string snapshot_dir,
                                                            std::string file_prefix);

}  // namespace raftdemo
