# Phase 1 Report: Durability Contract

## 本阶段读取了哪些文件

本阶段按受限范围读取了以下文件：

- `AGENTS.md`
- `modules/raft/storage/AGENTS.md`
- `modules/raft/node/AGENTS.md`
- `modules/raft/state_machine/AGENTS.md`
- `modules/raft/common/config.h`
- `modules/raft/storage/raft_storage.h`
- `modules/raft/storage/raft_storage.cpp`
- `modules/raft/storage/snapshot_storage.h`
- `modules/raft/storage/snapshot_storage.cpp`
- `modules/raft/node/raft_node.h`
- `modules/raft/node/raft_node.cpp`
- `modules/raft/state_machine/state_machine.h`
- `modules/raft/state_machine/state_machine.cpp`
- `tests/CMakeLists.txt`
- `proto/raft.proto`
- `tests/persistence_test.cpp`
- `tests/snapshot_test.cpp`
- `tests/test_raft_segment_storage.cpp`
- `tests/test_snapshot_storage_reliability.cpp`
- `tests/test_raft_snapshot_restart.cpp`
- `tests/test_raft_snapshot_diagnosis.cpp`

本阶段未读取：

- `NOTREAD.md`
- `README.md`
- `deploy/`
- `.gitignore` 覆盖目录中的内容

## 发现了哪些 durability 风险

本阶段确认的主要 durability 风险包括：

- 当前持久化路径没有显式 `fsync`
- 当前持久化路径没有 directory `fsync`
- `log/` 与 `meta.bin` 的发布顺序存在 crash window
- snapshot 直接写入最终目录，缺少单独 publish point
- `PruneSnapshots()` 删除旧 snapshot 后没有 crash consistency 说明
- power loss 下，acked write 不能被证明已经 durable

## 创建/更新了哪些文档

本阶段新增或更新了以下文档：

- `docs/PERSISTENCE_DURABILITY_CONTRACT.md`
- `specs/003-persistence-reliability/spec.md`
- `specs/003-persistence-reliability/plan.md`
- `specs/003-persistence-reliability/tasks.md`
- `specs/003-persistence-reliability/progress.md`
- `specs/003-persistence-reliability/decisions.md`
- `specs/003-persistence-reliability/phase-reports/phase-1-durability-contract.md`

## 后续 Phase 2 应该从哪里开始

Phase 2 建议从以下入口开始：

- `modules/raft/storage/raft_storage.cpp::WriteMeta`
- `modules/raft/storage/raft_storage.cpp::WriteSegments`
- `modules/raft/storage/raft_storage.cpp::ReplaceDirectory`
- `modules/raft/storage/raft_storage.cpp::ReplaceFile`
- `modules/raft/node/raft_node.cpp::PersistStateLocked`

原因是这些位置构成了 hard state 与 segmented log 的 durability fan-out 点，后续 `fsync` 语义和 publish ordering 收敛都需要从这里落地。

## 明确确认没有修改业务代码

本阶段仅新增/更新文档；未修改业务逻辑、协议语义、持久化格式、KV 逻辑或测试行为。
