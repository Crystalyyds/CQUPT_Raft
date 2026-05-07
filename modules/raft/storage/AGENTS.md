# Scope

负责 Raft 持久化边界：硬状态、segment log、snapshot catalog。

## Files

- `raft_storage.h`
- `raft_storage.cpp`
- `snapshot_storage.h`
- `snapshot_storage.cpp`

## Responsibilities

- `meta.bin` 读写
- segment log 读写、校验、截断与替换
- legacy `raft_state.bin` 兼容加载
- snapshot 目录保存、枚举、校验和 prune

## Out of Scope

- 不负责选举与复制决策
- 不负责 KV apply 语义
- 不负责 protobuf 契约

## Dependencies

- 允许依赖：标准库
- 当前 `raft_storage.cpp` 结构上依赖 `raft/node/raft_node.h` 中的 `LogRecord`
- 不应该依赖：`apps`、`raft/service`

## Change Rules

- 不允许修改持久化格式
- 不允许改 checksum、meta、segment 语义
- 这里只能做路径、include、构建层维护，除非明确要求改格式并同步迁移

## Relevant Tests

- `tests/test_raft_segment_storage.cpp`
- `tests/test_snapshot_storage_reliability.cpp`
- `tests/persistence_test.cpp`
- `tests/snapshot_test.cpp`
- `tests/test_raft_snapshot_restart.cpp`

## Risk Areas

- `meta.bin`
- segment 边界与 tail truncation
- snapshot 有效性选择
- prune 旧 snapshot

## Context Hints

- 先区分是 `raft_storage` 还是 `snapshot_storage`
- 修改前优先看对应测试
- 不要默认扫描 `raft/service`
