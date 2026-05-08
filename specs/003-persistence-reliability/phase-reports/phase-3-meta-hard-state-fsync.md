# Phase 3 Report: Meta Hard State Fsync

## 本阶段范围

本阶段只处理以下内容：

- `modules/raft/storage/raft_storage.cpp::WriteMeta`
- `modules/raft/storage/raft_storage.cpp::ReplaceFile`
- `modules/raft/node/raft_node.cpp::PersistStateLocked`
- `meta.bin` / hard state 相关 persistence 测试
- restart recovery 相关测试

本阶段未处理：

- segment log append / truncate
- `WriteSegments`
- `ReplaceDirectory`
- snapshot publish
- KV 逻辑
- proto / RPC
- transport
- dynamic membership
- 持久化格式变更
- 无关重构

## 代码修改摘要

### `modules/raft/storage/raft_storage.cpp`

- `WriteMeta`
  - 在保持现有 `meta.bin` 编码格式不变的前提下，补充了 temp meta file 的 file fsync 语义
  - 当前顺序变为：写入 `meta.bin.tmp` -> `flush` -> `close` -> `SyncFile(meta.bin.tmp)`
- `ReplaceFile`
  - 在 `meta.bin.tmp -> meta.bin` rename 成功后，补充了 parent directory `SyncDirectory`
  - 因此 `meta.bin` publish 成功返回前，目录项发布也要经过 directory durability 路径

### `modules/raft/node/raft_node.cpp`

- `PersistStateLocked`
  - 保持原有错误返回路径不变
  - 额外确保当 storage save 失败但 `reason` 为空时，填充默认错误消息，避免硬状态持久化失败被静默吞掉

## 测试修改摘要

### `tests/test_raft_segment_storage.cpp`

新增：

- `SavePublishesMetaFileWithoutLeavingMetaTempFile`
- `MissingMetaFileCausesLoadToReportNoPersistedState`
- `CorruptedMetaFileFailsLoad`

这些测试覆盖：

- `meta.bin` publish 后最终文件可见性
- `meta.bin` 缺失时的现有恢复行为
- `meta.bin` 损坏时的现有失败行为

### `tests/persistence_test.cpp`

新增：

- `ColdRestartPreservesPersistedHardStateBeforeStart`

该测试覆盖：

- `term` / `vote` 冷重启后的恢复表现
- `commit_index` / `last_applied` 冷重启后的恢复表现
- 在节点 `Start()` 前，仅通过构造恢复路径验证 persisted hard state

## 测试执行

执行了以下测试：

- `cmake --build --preset debug-ninja-low-parallel --target test_raft_segment_storage persistence_test snapshot_test test_raft_snapshot_restart test_raft_snapshot_diagnosis`
- `ctest --test-dir build --output-on-failure -R "RaftSegmentStorageTest|PersistenceTest|RaftSnapshotRecoveryTest|RaftSnapshotRestartTest|RaftSnapshotDiagnosisTest"`
- `CTEST_PARALLEL_LEVEL=1 ./test.sh --group persistence`

结果：

- 18 个相关 `ctest` 用例全部通过
- `./test.sh --group persistence` 的 3 个用例全部通过

## 结果与边界确认

- 本阶段没有修改持久化格式
- 本阶段没有修改 snapshot publish
- 本阶段没有修改 KV
- 本阶段没有修改 proto / RPC
- 本阶段没有修改 transport
- 本阶段没有修改 segment log append / truncate 逻辑
- 本阶段没有修改 `WriteSegments`
- 本阶段没有修改 `ReplaceDirectory`

## Windows 与跨平台状态

- POSIX/Linux：
  - `WriteMeta` file durability 通过真实 `fsync` 路径实现
  - `ReplaceFile` publish 后 directory durability 通过真实 directory `fsync` 路径实现
- Windows：
  - 继续沿用 Phase 2 已建立的 `FlushFileBuffers` / directory handle 路径
  - 当前没有引入新的 `_WIN32` no-op success 分支
  - 当前状态：未在 Windows 环境验证

## 未解决问题

- Windows runtime 语义仍未在实机或 CI 上验证
- snapshot publish 的 file / directory durability 仍未处理
- restart recovery diagnostics 仍未增强
- 故障注入与 crash simulation 仍未建立

这些内容应进入后续 Phase 4 及之后的阶段。

## Phase 4 建议入口

- `modules/raft/storage/snapshot_storage.cpp`
- `modules/raft/state_machine/state_machine.cpp`
- `modules/raft/node/raft_node.cpp` 中与 snapshot publish / recovery 编排直接相关的路径

原因：

- Phase 4 的核心问题是 snapshot publish atomicity，而不是 `meta.bin` 或 segment log durability
