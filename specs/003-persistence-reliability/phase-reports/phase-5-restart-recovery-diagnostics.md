# Phase 5B: Restart Recovery Validation And Diagnostics

## 1. 本阶段修改了哪些文件

- `modules/raft/storage/raft_storage.cpp`
- `modules/raft/storage/snapshot_storage.h`
- `modules/raft/storage/snapshot_storage.cpp`
- `modules/raft/node/raft_node.cpp`
- `tests/test_raft_segment_storage.cpp`
- `tests/test_snapshot_storage_reliability.cpp`
- `tests/persistence_test.cpp`
- `specs/003-persistence-reliability/tasks.md`
- `specs/003-persistence-reliability/progress.md`
- `specs/003-persistence-reliability/decisions.md`
- `specs/003-persistence-reliability/phase-reports/phase-5-restart-recovery-diagnostics.md`

## 2. 实现内容

`raft_storage.cpp` 增强了 `ReadMeta` / `LoadSegmented` / `LoadSegments` 的恢复错误上下文。meta 字段读取失败现在包含 path、field、reason；meta magic/version 错误包含 path 和实际值；`log_count`、`first_log_index`、`last_log_index` 的 boundary invariant 会在读取 meta 后立即校验。segment 加载失败会带上 log_dir、expected/actual count、expected/actual boundary 和 segment file 列表。

segment tail corruption 恢复路径现在记录诊断内容，包括坏尾原因、segment path、truncate offset、accepted record count，以及坏尾后被清理的 later segment。该改动不改变 segment 文件格式，不改变 append/truncate fsync 语义。

`snapshot_storage.h/.cpp` 增加 `SnapshotValidationIssue`、`SnapshotListResult` 和 `ListSnapshotsWithDiagnostics`。原有 `ListSnapshots` 行为保持不变，仍只返回 valid snapshots；新接口额外暴露 staging 目录忽略、缺失 meta、缺失 data、checksum mismatch 等 validation issue。

`raft_node.cpp` 在 startup recovery 中使用 `ListSnapshotsWithDiagnostics` 输出 snapshot scan summary、catalog skip reason、最终 loaded snapshot 状态，以及无可用 snapshot 时的 recovery 状态。constructor 完成后输出统一 restart recovery summary；commit/apply 越界 clamp 和 replay failure 也增加 commit/apply/snapshot/log 边界上下文。

## 3. 测试补充

- `tests/test_raft_segment_storage.cpp` 增加 unsupported meta version 诊断测试。
- `tests/test_raft_segment_storage.cpp` 增加 meta log boundary invariant 测试。
- `tests/test_raft_segment_storage.cpp` 增加 earlier segment tail corruption 后清理 later segment 并报告诊断的测试。
- `tests/test_snapshot_storage_reliability.cpp` 增加 snapshot validation issue 测试，覆盖 staging、缺失 meta、缺失 data、checksum mismatch。
- `tests/persistence_test.cpp` 增加 cold restart 时 commit_index / last_applied 越界 clamp 到 last log index 的测试。

## 4. 运行的测试

- `cmake --build --preset debug-ninja-low-parallel`
- `./build/tests/test_raft_segment_storage --gtest_filter='RaftSegmentStorageTest.TruncatesCorruptedSegmentTailDuringRecovery:RaftSegmentStorageTest.CorruptedEarlierSegmentTailCleansLaterSegmentsAndReportsDiagnostics:RaftSegmentStorageTest.UnsupportedMetaVersionFailsLoadWithPathAndVersion:RaftSegmentStorageTest.InconsistentMetaLogBoundaryFailsBeforeTrustingSegments:RaftSegmentStorageTest.CorruptedMetaFileFailsLoad:RaftSegmentStorageTest.MissingMetaFileCausesLoadToReportNoPersistedState'`
- `./build/tests/test_snapshot_storage_reliability`
- `./build/tests/persistence_test --gtest_filter='PersistenceTest.ColdRestartPreservesPersistedHardStateBeforeStart:PersistenceTest.ColdRestartClampsCommitAndApplyBoundariesToLastLogIndex:PersistenceTest.FullClusterRestartRecovery:PersistenceTest.RestartedFollowerCatchesUp'`
- `./build/tests/test_raft_snapshot_restart --gtest_filter='RaftSnapshotRestartTest.*'`
- `./build/tests/test_raft_snapshot_diagnosis --gtest_filter='RaftSnapshotDiagnosisTest.*'`
- `./build/tests/snapshot_test --gtest_filter='RaftSnapshotRecoveryTest.SavesSnapshotAndRestoresAfterRestart'`

结果：

- 构建通过。
- segment storage 相关 6 个过滤测试通过。
- snapshot storage reliability 7 个测试通过。
- persistence 相关 4 个过滤测试通过。
- snapshot restart 4 个测试通过。
- snapshot diagnosis 2 个测试通过。
- `RaftSnapshotRecoveryTest.SavesSnapshotAndRestoresAfterRestart` 本次单独运行通过。

## 5. 持久化格式与范围确认

本阶段未修改 `meta.bin`、segment log、snapshot data 或 snapshot metadata 的字段、编码、文件名或目录命名。

本阶段未修改 segment log fsync 语义、meta fsync 语义或 snapshot atomic publish 语义。

本阶段未修改 proto / RPC / KV / transport / dynamic membership。

## 6. 跨平台注意事项

Phase 5B 没有新增 required durability operation，因此没有新增平台 flush 分支。已有 POSIX/Linux fsync 与 Windows `FlushFileBuffers` 约束保持不变。当前测试在 POSIX/Linux 环境验证；Windows 运行时语义未在本阶段实机或 CI 验证。

## 7. 未解决问题

- Phase 5B 没有引入 crash / failure injection；这仍属于 Phase 6。
- snapshot catalog diagnostics 通过新接口暴露 validation issues，但没有引入结构化日志 sink；当前 node 侧仍通过现有 `Log` 输出恢复摘要。
