# Scope

负责 KV 状态机与状态机快照格式。

## Files

- `state_machine.h`
- `state_machine.cpp`

## Responsibilities

- 应用 `SET` / `DEL` 命令
- 提供 `Get` 与 `DebugString`
- 保存和加载状态机 snapshot

## Out of Scope

- 不负责 quorum、term、vote、leader
- 不负责 snapshot catalog 管理
- 不负责 gRPC 适配

## Dependencies

- 允许依赖：`raft/common`
- 不应该依赖：`raft/service`、`apps`

## Change Rules

- 不要改命令语义
- 不要改状态机 snapshot 二进制格式
- 路径调整之外，不要在这里补 Raft 逻辑

## Relevant Tests

- `tests/test_state_machine.cpp`
- `tests/test_raft_commit_apply.cpp`
- `tests/snapshot_test.cpp`
- `tests/test_raft_snapshot_restart.cpp`

## Risk Areas

- snapshot 文件头
- key/value 序列化顺序
- noop 命令处理

## Context Hints

- 先读 `state_machine.h`
- 再读 `Apply`、`SaveSnapshot`、`LoadSnapshot`
- 不需要时不要默认进入 `raft/node`
