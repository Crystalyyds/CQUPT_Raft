# 003-persistence-reliability Plan

## 总体计划

- `Phase 1: durability contract`
- `Phase 2: storage log append / truncate fsync semantics`
- `Phase 3: meta hard state fsync semantics`
- `Phase 4: snapshot atomic publish semantics`
- `Phase 5: restart recovery validation and diagnostics`
- `Phase 6: crash / failure injection tests`

## Phase 1

### 输入来源

- 根 `AGENTS.md`
- `modules/raft/storage/AGENTS.md`
- `modules/raft/node/AGENTS.md`
- `modules/raft/state_machine/AGENTS.md`
- `modules/raft/common/config.h`
- `modules/raft/storage/raft_storage.h/.cpp`
- `modules/raft/storage/snapshot_storage.h/.cpp`
- `modules/raft/node/raft_node.h/.cpp`
- `modules/raft/state_machine/state_machine.h/.cpp`
- `tests/CMakeLists.txt`
- `proto/raft.proto`
- `tests/persistence_test.cpp`
- `tests/snapshot_test.cpp`
- `tests/test_raft_segment_storage.cpp`
- `tests/test_snapshot_storage_reliability.cpp`
- `tests/test_raft_snapshot_restart.cpp`
- `tests/test_raft_snapshot_diagnosis.cpp`

### 产出物

- `docs/PERSISTENCE_DURABILITY_CONTRACT.md`
- `specs/003-persistence-reliability/spec.md`
- `specs/003-persistence-reliability/plan.md`
- `specs/003-persistence-reliability/tasks.md`
- `specs/003-persistence-reliability/progress.md`
- `specs/003-persistence-reliability/decisions.md`
- `specs/003-persistence-reliability/phase-reports/phase-1-durability-contract.md`

### 要明确的风险分类

- file `fsync` 缺口
- directory `fsync` 缺口
- publish ordering 风险
- trusted-state 判定边界
- power loss 下未严格保证的 durability 语义

### 交付标准

- 文档清楚描述 meta、log、snapshot 的写入路径和恢复路径
- 文档清楚描述正常退出、进程 crash、power loss 下的目标保证与非保证项
- 文档清楚描述 restart recovery 如何判断可信状态
- 文档清楚区分当前实现现状与后续阶段收敛目标

## Phase 2

目标：

- 只为 segment log append / truncate 路径补齐 durability 语义
- 让 log append / truncate 在成功返回后具备更明确的 durability 边界
- 收敛 `log/` 目录 publish 的 crash window 与目录项可见性语义
- 为 segment log append / truncate / recovery 相关测试建立明确验证面

范围内：

- segment log append
- segment log truncate
- `log/` directory publish
- `modules/raft/storage/raft_storage.cpp::WriteSegments`
- `modules/raft/storage/raft_storage.cpp::ReplaceDirectory`
- segment log append / truncate / recovery 相关测试

范围外：

- `meta.bin`
- hard state
- `term`
- `vote`
- `commit_index`
- `last_applied`
- `WriteMeta`
- `ReplaceFile`，除非后续确认它被 segment log 路径直接使用
- `PersistStateLocked`
- `modules/raft/node`
- snapshot publish
- KV 逻辑
- proto / RPC
- 持久化格式变更

建议入口：

- `modules/raft/storage/raft_storage.cpp::WriteSegments`
- `modules/raft/storage/raft_storage.cpp::ReplaceDirectory`
- `tests/test_raft_segment_storage.cpp`

原因：

- Phase 2 只收敛 segment log durability，入口应局限在 `storage` 中真正负责 segment 文件和 `log/` 目录发布的位置
- 先把 segment log append / truncate / recovery 路径切干净，再进入 meta / hard state，可避免一次修改跨 `storage log`、`meta hard state` 和 `node core`

## Phase 3

目标：

- 为 hard state 的 `term`、`vote`、`commit_index`、`last_applied` 建立更严格的 durability 语义
- 让 `meta.bin` 的成功返回更接近可证明 durable 的承诺
- 明确 hard state 与 log 边界的一致发布要求

范围内：

- `meta.bin`
- hard state
- `term`
- `vote`
- `commit_index`
- `last_applied`
- `modules/raft/storage/raft_storage.cpp::WriteMeta`
- `modules/raft/storage/raft_storage.cpp::ReplaceFile`
- `modules/raft/node/raft_node.cpp::PersistStateLocked`

建议入口：

- `modules/raft/storage/raft_storage.cpp::WriteMeta`
- `modules/raft/storage/raft_storage.cpp::ReplaceFile`
- `modules/raft/node/raft_node.cpp::PersistStateLocked`

原因：

- 这些位置共同决定 hard state 与 `meta.bin` 的 durable publish 语义
- 把它们放到 Phase 3，可以避免在 Phase 2 同时处理 segment log、meta publish 和 node core 调用边界

## Phase 4

目标：

- 为 snapshot publish 建立更强的原子性语义
- 消除“直接写最终 snapshot 目录”的主要发布窗口
- 为 snapshot data、snapshot metadata 和目录项建立一致的 publish contract

## Phase 5

目标：

- 强化 restart recovery 的 trusted-state 校验
- 补充更明确的恢复失败诊断和状态说明
- 把 meta/log/snapshot 的恢复判定规则显式化，便于测试和排障

## Phase 6

目标：

- 引入 crash / failure injection 测试场景
- 覆盖 meta、log、snapshot publish 的关键崩溃窗口
- 验证 power loss 近似场景下的回退、拒绝和恢复行为

## 涉及模块

- `modules/raft/storage`
- `modules/raft/node`
- `modules/raft/state_machine`
- `modules/raft/common`
- `tests`

## 风险

- `meta.bin` 与 `log/` 的发布顺序目前存在 crash window
- snapshot 直接写最终目录，缺少单独 publish point
- 缺少 `fsync` 和 directory `fsync`，power loss 下语义不明确
- compaction、snapshot publish、restart recovery 三条路径相互耦合，后续修改风险高

## 验证方式

- Phase 1：文档自检，确保每条结论都能回溯到源码或代表性测试
- Phase 2-4：单元测试 + persistence/snapshot/restart 相关回归测试
- Phase 5：恢复判定和诊断输出测试
- Phase 6：crash/failure injection 测试
