# Scope

负责 gRPC Raft/KV 服务适配层。

## Files

- `raft_service_impl.h`
- `raft_service_impl.cpp`
- `kv_service_impl.h`
- `kv_service_impl.cpp`

## Responsibilities

- 接收 gRPC 回调
- 调用 `RaftNode`
- 回填 protobuf 响应
- 记录 RPC 延迟指标

## Out of Scope

- 不拥有 Raft 共识状态
- 不定义 protobuf schema
- 不定义持久化行为

## Dependencies

- 允许依赖：`proto`、`raft/node`、`raft/common`
- 不应该依赖：`apps`

## Change Rules

- 不要改变 RPC 语义
- 不要改状态码含义
- 这里只改适配层和路径，不把业务逻辑塞进 service 层

## Relevant Tests

- `tests/test_kv_service.cpp`
- `tests/raft_integration_test.cpp`

## Risk Areas

- leader redirect 语义
- status/metrics 字段填充
- Raft RPC 与 KV RPC 的 protobuf 对齐

## Context Hints

- 先读 `proto/raft.proto`
- 再读对应 service 实现
- 需要行为确认时再进入 `raft/node`
