# Scope

负责运行时基础设施：日志、定时器、线程池。

## Files

- `logging.h`
- `min_heap_timer.h`
- `min_heap_timer.cpp`
- `thread_pool.h`
- `thread_pool.cpp`

## Responsibilities

- 提供线程安全日志输出
- 提供定时调度器
- 提供基础任务线程池

## Out of Scope

- 不负责 Raft 状态机语义
- 不负责持久化格式
- 不负责 RPC 契约

## Dependencies

- 允许依赖：标准库
- 不应该依赖：`raft/node`、`raft/storage`、`raft/service`

## Change Rules

- 优先保持 API 稳定
- 修改并发原语前必须检查所有调用点

## Relevant Tests

- `tests/test_min_heap_timer.cpp`
- `tests/test_thread_pool.cpp`

## Risk Areas

- 竞态条件
- stop/shutdown 语义
- 定时器取消与回调并发

## Context Hints

- 定位问题先从对应 `.h` 与 `.cpp` 成对阅读
- 除非调用关系需要，不要默认扫描整个 Raft 核心
