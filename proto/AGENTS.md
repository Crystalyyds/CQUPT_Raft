# Scope

负责 RPC/Protobuf 契约层。

## Files

- `raft.proto`

## Responsibilities

- 定义 `RaftService`
- 定义 `KvService`
- 定义相关 request/response/message/enum

## Out of Scope

- 不负责业务实现
- 不负责持久化格式
- 不负责测试调度

## Dependencies

- 允许依赖：无源码级依赖
- 不应该依赖：任何 C++ 模块

## Change Rules

- 高风险区域
- 不要修改协议语义，除非任务明确要求并同步更新所有调用方与测试
- 字段编号、消息名、状态码都视为稳定契约

## Relevant Tests

- `tests/test_kv_service.cpp`
- `tests/raft_integration_test.cpp`
- 其余绝大部分集群测试都会间接依赖这里

## Risk Areas

- message 字段编号
- 状态码枚举
- Raft RPC 与 KV RPC 的兼容性

## Context Hints

- 修改前先确认是不是必须动 `proto`
- 若只是实现层问题，不要进入该目录
