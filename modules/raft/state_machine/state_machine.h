#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

#include "raft/command.h"

namespace raftdemo
{
    struct ApplyResult
    {
        bool Ok{false};
        std::string message;
    };

    enum class SnapshotStatus : std::uint8_t
    {
        kOk = 0,
        kInvalidArgument = 1,
        kIoError = 2,
        kCorruptedData = 3,
        kNotFound = 4,
        kVersionMismatch = 5,
        kInternalError = 6
    };

    struct SnapshotResult
    {
        SnapshotStatus status{SnapshotStatus::kInternalError};
        std::string message;

        bool Ok() const
        {
            return status == SnapshotStatus::kOk;
        }
    };

    class IStateMachine
    {
    public:
        virtual ~IStateMachine() = default;

        virtual ApplyResult Apply(std::uint64_t index,
                                  const std::string &command_data) = 0;

        virtual SnapshotResult SaveSnapshot(const std::string &file_path) const = 0;
        virtual SnapshotResult LoadSnapshot(const std::string &file_path) = 0;
    };

    class KvStateMachine final : public IStateMachine
    {
    public:
        ApplyResult Apply(std::uint64_t index,
                          const std::string &command_data) override;

        SnapshotResult SaveSnapshot(const std::string &file_path) const override;
        SnapshotResult LoadSnapshot(const std::string &file_path) override;

        bool Get(const std::string &key, std::string *value) const;
        std::string DebugString() const;

    private:
        mutable std::mutex mu_;
        std::unordered_map<std::string, std::string> kv_;
    };
} // namespace raftdemo