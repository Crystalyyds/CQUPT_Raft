#include "raft/state_machine.h"

#include <algorithm>
#include <sstream>
#include <vector>

namespace raftdemo
{
    ApplyResult KvStateMachine::Apply(std::uint64_t /*index*/,
                                      const std::string &command_data)
    {
        Command cmd;
        if (!Command::Deserialize(command_data, &cmd))
        {
            return {false, "failed to deserialize command"};
        }

        if (!cmd.IsValid())
        {
            return {false, "invalid command"};
        }

        std::lock_guard<std::mutex> lk(mu_);

        switch (cmd.type)
        {
        case CommandType::kSet:
            kv_[cmd.key] = cmd.value;
            return {true, "ok"};

        case CommandType::kDelete:
            kv_.erase(cmd.key);
            return {true, "ok"};

        case CommandType::kUnknown:
        default:
            return {false, "unknown command type"};
        }
    }

    bool KvStateMachine::Get(const std::string &key, std::string *value) const
    {
        if (value == nullptr)
        {
            return false;
        }

        std::lock_guard<std::mutex> lk(mu_);
        auto it = kv_.find(key);
        if (it == kv_.end())
        {
            return false;
        }

        *value = it->second;
        return true;
    }

    std::string KvStateMachine::DebugString() const
    {
        std::lock_guard<std::mutex> lk(mu_);

        std::vector<std::pair<std::string, std::string>> items(kv_.begin(), kv_.end());
        std::sort(items.begin(), items.end(),
                  [](const auto &a, const auto &b)
                  { return a.first < b.first; });

        std::ostringstream oss;
        oss << "{";
        for (std::size_t i = 0; i < items.size(); ++i)
        {
            if (i > 0)
            {
                oss << ", ";
            }
            oss << items[i].first << "=" << items[i].second;
        }
        oss << "}";
        return oss.str();
    }

} // namespace raftdemo