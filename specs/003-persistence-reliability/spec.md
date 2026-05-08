# 003-persistence-reliability

## Goal

为当前 CQUPT_Raft 项目建立明确的 persistence durability contract，把 `meta.bin`、segment log、snapshot data、snapshot metadata 在正常退出、进程 crash、power loss、restart recovery 下的现状能力、缺口和后续修补方向写成可执行规范。

## Requirements

- 覆盖 `meta.bin`、`log/segment_*.log`、`snapshot_<index>/data.bin`、`snapshot_<index>/__raft_snapshot_meta`
- 覆盖 normal exit、process crash、power loss、restart recovery
- 明确 file `fsync`、directory `fsync`、publish ordering 风险
- 明确 hard state、log append / truncate、snapshot publish 的 durability 语义
- 明确 restart recovery 的 trusted-state 判定规则
- 明确当前阶段必须保证什么，以及当前阶段暂不保证什么
- 明确后续 Phase 2-6 的工作目标和测试补齐方向

## Non-goals

- 不修改业务代码
- 不修改持久化格式
- 不修改 Raft 协议语义
- 不修改 KV 逻辑
- 不引入新的 storage engine
- 不实现 `fsync`、directory `fsync` 或 crash injection 机制

## Acceptance Criteria

- 每个持久化对象都有明确的磁盘路径、写入入口、恢复入口
- 每类 durability gap 都被明确分类为 `fsync`、directory `fsync`、publish ordering 或可信状态判定问题
- restart recovery 的 trusted-state 规则可以直接转化为后续测试场景
- Phase 1 文档明确区分“当前实现现状”和“后续阶段目标 contract”
- Phase 2-6 已在计划中排布，且仅定义目标，不提前实现

## Scope

- `modules/raft/storage`
- `modules/raft/node`
- `modules/raft/state_machine`
- `modules/raft/common/config.h`
- `proto/raft.proto`
- `tests/persistence_test.cpp`
- `tests/snapshot_test.cpp`
- `tests/test_raft_segment_storage.cpp`
- `tests/test_snapshot_storage_reliability.cpp`
- `tests/test_raft_snapshot_restart.cpp`
- `tests/test_raft_snapshot_diagnosis.cpp`

## Out of Scope

- 真正实现 `fsync`
- 替换或重构 storage engine
- snapshot 性能优化
- KV / service / proto 语义调整
- 新增或修改持久化格式
- 新增 crash/failure injection 基础设施
