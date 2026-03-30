#include "raft/command.h"

#include <sstream>
#include <vector>

namespace raftdemo
{
    namespace
    {
        // 反序列化内部函数，(***,)
        std::vector<std::string> Split(const std::string &str, char delim)
        {
            std::vector<std::string> parts;
            std::stringstream ss(str);
            std::string item;

            while (std::getline(ss, item, delim))
            {
                parts.push_back(item);
            }

            return parts;
        }
    } // namespace

    bool Command::IsValid() const
    {
        switch (type)
        {
        case CommandType::kSet:
            return !key.empty();
        case CommandType::kDelete:
            return !key.empty();
        case CommandType::kUnknown:
        default:
            return false;
        }
    }

    // 这里自定义协议
    std::string Command::Serialize() const
    {
        // 简单协议
        // Set    -> "SET|key|value"
        // Delete -> "DEL|key|"

        switch (type)
        {
        case CommandType::kSet:
            return "SET|" + key + "|" + value;
        case CommandType::kDelete:
            return "DEL|" + key + "|";
        case CommandType::kUnknown:
        default:
            return "";
        }
    }

    bool Command::Deserialize(const std::string &data, Command *out)
    {
        if (out == nullptr)
        {
            return false;
        }

        const std::vector<std::string> parts = Split(data, '|');
        if (parts.size() < 3)
            return false;

        if (parts[0] == "SET")
        {
            out->type = CommandType::kSet;
            out->key = parts[1];
            out->value = parts[2];
            return out->IsValid();
        }
        else if (parts[0] == "DEL")
        {
            out->type = CommandType::kDelete;
            out->key = parts[1];
            out->value = "";
            return out->IsValid();
        }
        else
        {
            return false;
        }
    }

} // namespace raftdemo