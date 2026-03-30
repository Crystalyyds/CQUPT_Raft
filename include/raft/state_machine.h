#pragma once

#include <string>
#include <cstdint>

namespace raftdemo
{
    // 状态机执行结果
    struct ApplyResult
    {
        bool Ok{false};
        std::string message;
    };

    // 状态机抽象接口。
    class IStateMachine
    {
    public:
        /**
         * @brief 将一条已提交日志应用到状态机。
         *
         * 注意：只有已经 commit 的日志，才能调用这个接口。
         *
         * @param index        日志下标
         * @param command_data 日志中保存的业务命令数据
         * @return ApplyResult 状态机执行结果
         */
        virtual ApplyResult Apply(std::uint64_t index,
                                  const std::string &command_data) = 0;
    };
} // namespace raftdemo