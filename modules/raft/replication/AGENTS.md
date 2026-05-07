# Scope

负责单 follower 的复制状态机。

## Files

- `replicator.h`
- `replicator.cpp`

## Responsibilities

- 构造单次 `AppendEntries`
- 处理复制响应
- 管理 per-peer backoff 与 inflight 状态
- 在需要时切换到 `InstallSnapshot`

## Out of Scope

- 不负责选举
- 不负责状态机 apply 逻辑
- 不负责 segment log 或 snapshot 文件格式

## Dependencies

- 允许依赖：`raft/common`、`raft/node`、`raft/runtime`、protobuf 生成头
- 不应该依赖：`apps`

## Change Rules

- 不要改冲突回退语义
- 不要改批量复制和 snapshot 切换语义
- 修改这里时必须回看相关 follower catch-up 测试

## Relevant Tests

- `tests/test_raft_replicator_behavior.cpp`
- `tests/test_raft_log_replication.cpp`
- `tests/test_raft_snapshot_catchup.cpp`
- `tests/test_raft_split_brain.cpp`

## Risk Areas

- `next_index` 回退
- conflict hint 使用
- snapshot 安装切换条件
- transport failure backoff

## Context Hints

- 先读 `replicator.h`
- 再读 `ReplicateOnce`、`BuildAppendEntriesRequest`、`HandleAppendEntriesResponse`
- 只在需要全局状态时再查看 `raft/node`
