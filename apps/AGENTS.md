# Scope

负责可执行入口层。

## Files

- `main.cpp`
- `raft_kv_client.cpp`

## Responsibilities

- 节点进程启动
- 文本配置解析
- CLI 客户端命令发起

## Out of Scope

- 不负责共识实现
- 不负责持久化格式
- 不负责 protobuf schema 定义

## Dependencies

- 允许依赖：`raft/common`、`raft/runtime`、`raft/node`、protobuf 生成头
- 不应该依赖：测试目录

## Change Rules

- 不要在入口层引入业务逻辑分叉
- 优先把复杂逻辑留在模块内，入口层保持薄

## Relevant Tests

- `tests/test_kv_service.cpp`
- `tests/raft_integration_test.cpp`
- `scripts/acceptance_cluster.sh`

## Risk Areas

- 配置字段解析
- 启动参数约定
- CLI 输出格式被脚本依赖

## Context Hints

- 改服务端入口先读 `main.cpp`
- 改客户端行为先读 `raft_kv_client.cpp`
- 涉及 RPC 语义时再去 `proto/` 和 `modules/raft/service`
