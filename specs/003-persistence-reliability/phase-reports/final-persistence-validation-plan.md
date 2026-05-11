# Final Persistence Validation Plan

## 1. 读取了哪些文件

- `AGENTS.md`
- `modules/raft/storage/AGENTS.md`
- `modules/raft/node/AGENTS.md`
- `modules/raft/state_machine/AGENTS.md`
- `docs/PERSISTENCE_DURABILITY_CONTRACT.md`
- `specs/003-persistence-reliability/spec.md`
- `specs/003-persistence-reliability/plan.md`
- `specs/003-persistence-reliability/tasks.md`
- `specs/003-persistence-reliability/progress.md`
- `specs/003-persistence-reliability/decisions.md`
- `specs/003-persistence-reliability/phase-reports/phase-1-durability-contract.md`
- `specs/003-persistence-reliability/phase-reports/phase-2-storage-log-fsync.md`
- `specs/003-persistence-reliability/phase-reports/phase-3a-meta-hard-state-plan.md`
- `specs/003-persistence-reliability/phase-reports/phase-3-meta-hard-state-fsync.md`
- `specs/003-persistence-reliability/phase-reports/phase-4a-snapshot-atomic-publish-plan.md`
- `specs/003-persistence-reliability/phase-reports/phase-4-snapshot-atomic-publish.md`
- `specs/003-persistence-reliability/phase-reports/phase-5a-restart-recovery-diagnostics-plan.md`
- `specs/003-persistence-reliability/phase-reports/phase-5-restart-recovery-diagnostics.md`
- `specs/003-persistence-reliability/phase-reports/phase-6a-crash-failure-injection-plan.md`
- `specs/003-persistence-reliability/phase-reports/phase-6-crash-failure-injection-tests.md`

本次未读取 `NOTREAD.md`、`README.md`、`deploy/` 或 `.gitignore` 覆盖目录内容。

## 2. 各 Phase 完成情况

| Phase | 目标 | 是否完成 | 证据 | 未解决问题 | 风险等级 |
| --- | --- | --- | --- | --- | --- |
| Phase 1 | 建立 durability contract，描述 meta、log、snapshot、restart recovery 的现状和目标 | 已完成文档化 | `phase-1-durability-contract.md` 与 `docs/PERSISTENCE_DURABILITY_CONTRACT.md` 已创建 | contract 文档中的“当前实现现状”反映 Phase 1 初始状态，后续 Phase 2-4 已改变部分实现；最终发布前建议补一段“Phase 2-6 更新后状态” | Medium |
| Phase 2 | segment log append / truncate / log directory publish 的 file fsync 与 directory fsync | 已完成 targeted 实现与测试 | `phase-2-storage-log-fsync.md` 记录 `WriteSegments`、tail truncate、`ReplaceDirectory` 的 POSIX fsync 和 Windows `FlushFileBuffers` 路径；segment 与 persistence 测试通过 | Windows runtime 未验证；真实 power loss 未验证；精确 fsync / rename 失败未注入 | Medium |
| Phase 3 | `meta.bin` / hard state file fsync、publish directory fsync 与错误传播 | 已完成 targeted 实现与测试 | `phase-3-meta-hard-state-fsync.md` 记录 `WriteMeta`、`ReplaceFile`、`PersistStateLocked` 修改；相关 segment、persistence、snapshot recovery 测试通过 | Windows runtime 未验证；`meta.bin.tmp` 写入中途 short write、fsync failure、directory sync failure 仍缺精确注入测试 | Medium |
| Phase 4 | snapshot staged atomic publish、data/meta file fsync、directory fsync、trusted snapshot 过滤 | 已完成 targeted 实现与测试 | `phase-4-snapshot-atomic-publish.md` 记录 staged dir publish、`data.bin` / `__raft_snapshot_meta` fsync、`snapshot_dir` sync、同 index 幂等策略；snapshot reliability / restart / diagnosis / persistence 测试通过 | Windows runtime 未验证；同 index 不同 term 返回错误是显式取舍；state machine work-file 与 install temp file durability 不作为 committed snapshot publish 点覆盖 | Medium |
| Phase 5 | restart recovery validation 与 diagnostics，解释 meta/log/snapshot trusted-state 选择 | 已完成 targeted 实现与测试 | `phase-5-restart-recovery-diagnostics.md` 记录 meta/log 边界诊断、segment truncate reason、snapshot skip reason、startup recovery summary；相关测试通过 | 诊断通过现有日志与 storage diagnostic 接口暴露，没有结构化日志 sink；未新增 durability barrier | Low |
| Phase 6 | crash / failure injection tests 最小实现 | 部分完成 | `phase-6-crash-failure-injection-tests.md` 记录新增 crash artifact 测试：partial segment header、temp publish artifacts、old meta/new log、新 meta/old log、全部 invalid snapshot；相关测试通过 | T613-T616 deferred：精确 `fsync`、directory fsync、rename / replace、remove / prune、partial write failure injection 需要 test-only hook；真实 power loss 未验证 | High |

## 3. 遗留问题汇总

| 问题 | 来源 Phase | 涉及模块 | 影响 | 是否阻塞最终验收 | 建议处理方式 |
| --- | --- | --- | --- | --- | --- |
| Windows 分支没有实机或 CI runtime 验证 | Phase 2-4, 6 | `modules/raft/storage` | 不能声称跨平台 durability 已完全验证；只能说 Windows 代码路径已实现且未使用 no-op success | 阻塞跨平台最终验收；不阻塞当前 Linux/POSIX targeted 验收 | 增加 Windows CI 或手动 Windows 测试，覆盖 storage、meta、snapshot 和 restart recovery |
| 真实 power loss 未验证 | Phase 1, 6 | storage / node / tests | 当前测试是 crash-like disk artifact 近似，不能证明 kernel / disk 断电场景 | 阻塞 power-loss certification；不阻塞功能性 recovery 回归 | 后续引入进程 kill、虚拟磁盘或 fault-injection 环境；在结论中禁止宣称已证明 power-loss-safe |
| 精确 durability failure injection 缺口 | Phase 6 | `raft_storage.cpp`、`snapshot_storage.cpp`、tests | 无法稳定覆盖 `fsync`、directory fsync、rename、remove、partial write 的内部失败路径 | 阻塞严格 failure-injection 验收 | 进入 Phase 6C 时设计默认关闭的 test-only hook，不改变生产默认语义 |
| `docs/PERSISTENCE_DURABILITY_CONTRACT.md` 与后续实现状态可能存在时间漂移 | Phase 1-4 | docs/specs | Phase 1 文档记录初始缺口；后续实现已补 segment/meta/snapshot durability barrier，读者可能误解最终状态 | 阻塞文档最终发布质量；不阻塞代码测试执行 | 在最终收尾 commit 前更新 contract 的“当前实现状态”或追加“Phase 2-6 update”章节 |
| snapshot 同 index 不同 term 选择显式失败而非替换 | Phase 4 | snapshot storage | 避免非空目录跨平台替换 crash window，但同 index 不同 term 场景会返回错误 | 不阻塞当前 contract；属于已记录取舍 | 保持决策记录；如业务需要覆盖替换，单独设计平台兼容的 replace protocol |
| state machine work-file 与 install temp file 的 pre-publish durability 未作为 committed snapshot publish 点覆盖 | Phase 4 | state_machine / node / snapshot storage | publish 前的中间文件断电不承诺成为可信 snapshot；失败应通过 catalog publish 边界隔离 | 不阻塞 snapshot catalog durability 验收 | 在 contract 中明确“只有 catalog publish 后的 snapshot 是可信对象”；如需更强保证另开阶段 |
| full `./test.sh --group all` / full CTest 未在最终验收报告中执行 | Phase 2-6 | 全项目测试 | targeted 测试通过，但总回归尚未在最终验收中确认 | 阻塞最终合入前验收 | 按本文测试顺序执行完整回归，再决定提交 |
| sanitizer / TSAN / ASAN 未覆盖 | Phase 6 | 并发、restart、snapshot | 不能排除并发与内存错误类风险 | 不阻塞当前 persistence 功能验收；阻塞更高质量门禁 | 后续增加 sanitizer CI 或 nightly 测试 |

## 4. 未验证项汇总

- Windows 分支：POSIX 路径已在当前环境测试；Windows `FlushFileBuffers`、directory handle、rename / directory sync 运行时语义未在 Windows 实机或 CI 验证。
- Directory durability：POSIX `fsync` 已通过相关测试执行路径；Windows directory `FlushFileBuffers` 仅完成代码路径设计，未 runtime 验证。
- Power loss：现有测试主要通过坏文件、坏目录、temp 残留、checksum 损坏模拟 crash-like 状态，不等价于真实断电测试。
- 文件系统语义：rename、directory fsync、权限和 remove 行为存在平台差异；当前结果主要代表当前 POSIX/Linux 文件系统环境。
- Failure injection：T613-T616 deferred，未覆盖精确 syscall / filesystem 操作失败点。
- CI / sanitizer：阶段报告没有记录 Windows CI、ASAN、TSAN、UBSAN 或真实断电测试。

## 5. 最终测试顺序

### 第一层：编译和基础单测

测试目标：

- 确认当前工程可完整编译。
- 先排除基础模块失败，避免 persistence 测试结果被构建或基础单元问题干扰。

推荐命令：

```bash
cmake --preset debug-ninja-low-parallel
cmake --build --preset debug-ninja-low-parallel
ctest --test-dir build --output-on-failure -R "Command|StateMachine|Timer|Thread|Config"
```

通过标准：

- configure / build 成功。
- 基础单测全部通过，没有跳过或删除旧测试。

失败先看：

- `CMakeLists.txt`
- `tests/CMakeLists.txt`
- 失败测试对应模块源码与测试文件

是否阻塞最终验收：

- 是。

### 第二层：storage / segment log 测试

测试目标：

- 验证 segment append / truncate / `log/` publish durability 语义。
- 验证 crash artifact 下的 segment trusted-state：坏 tail 截断、`log.tmp/` / `log.bak/` 忽略、old meta/new log 边界。

推荐命令：

```bash
./build/tests/test_raft_segment_storage
```

通过标准：

- `RaftSegmentStorageTest` 全部通过。
- 重点用例包括 `TruncatesCorruptedSegmentTailDuringRecovery`、`TruncatesPartialSegmentHeaderDuringRecovery`、`RecoveryIgnoresTemporaryPublishArtifacts`、`MetaAndLogPublishWindowUsesOnlyTrustedBoundary`。

失败先看：

- `modules/raft/storage/raft_storage.cpp`
- `tests/test_raft_segment_storage.cpp`

是否阻塞最终验收：

- 是。

### 第三层：meta / hard state persistence 测试

测试目标：

- 验证 `meta.bin` file fsync / publish directory fsync 后的恢复边界。
- 验证 `term`、`vote`、`commit_index`、`last_applied` 的 cold restart 行为。
- 验证 `meta.bin` 缺失、损坏、unsupported version、boundary 不一致时的 failure / diagnostic 行为。

推荐命令：

```bash
./build/tests/persistence_test --gtest_filter='PersistenceTest.ColdRestartPreservesPersistedHardStateBeforeStart:PersistenceTest.ColdRestartClampsCommitAndApplyBoundariesToLastLogIndex'
./build/tests/test_raft_segment_storage --gtest_filter='RaftSegmentStorageTest.SavePublishesMetaFileWithoutLeavingMetaTempFile:RaftSegmentStorageTest.MissingMetaFileCausesLoadToReportNoPersistedState:RaftSegmentStorageTest.CorruptedMetaFileFailsLoad:RaftSegmentStorageTest.UnsupportedMetaVersionFailsLoadWithPathAndVersion:RaftSegmentStorageTest.InconsistentMetaLogBoundaryFailsBeforeTrustingSegments'
```

通过标准：

- hard state restart 恢复值符合预期。
- meta/log boundary 不一致不能被接受为 trusted state。

失败先看：

- `modules/raft/storage/raft_storage.cpp`
- `modules/raft/node/raft_node.cpp`
- `tests/persistence_test.cpp`
- `tests/test_raft_segment_storage.cpp`

是否阻塞最终验收：

- 是。

### 第四层：snapshot storage reliability 测试

测试目标：

- 验证 staged snapshot publish、`data.bin` / `__raft_snapshot_meta` file fsync、staging dir / `snapshot_dir` directory fsync 后的最终布局和 trusted-state。
- 验证 temp / incomplete / corrupted snapshot 不会被 catalog 视为 trusted snapshot。

推荐命令：

```bash
./build/tests/test_snapshot_storage_reliability
```

通过标准：

- `SnapshotStorageReliabilityTest` 全部通过。
- 重点覆盖 staging ignore、missing data、missing metadata、checksum mismatch、latest invalid fallback、all invalid returns no trusted snapshot。

失败先看：

- `modules/raft/storage/snapshot_storage.cpp`
- `modules/raft/storage/snapshot_storage.h`
- `tests/test_snapshot_storage_reliability.cpp`

是否阻塞最终验收：

- 是。

### 第五层：snapshot restart / diagnosis 测试

测试目标：

- 验证 snapshot startup recovery 的 latest-valid-first、invalid fallback、post-snapshot tail log replay。
- 验证 recovery diagnostics 能解释 skip reason、selected snapshot 和 post-state summary。

推荐命令：

```bash
./build/tests/test_raft_snapshot_restart
./build/tests/test_raft_snapshot_diagnosis
./build/tests/snapshot_test --gtest_filter='RaftSnapshotRecoveryTest.SavesSnapshotAndRestoresAfterRestart'
```

通过标准：

- snapshot restart / diagnosis 测试全部通过。
- 不出现通过删除、跳过或弱化 snapshot recovery 测试获得的结果。

失败先看：

- `modules/raft/node/raft_node.cpp`
- `modules/raft/storage/snapshot_storage.cpp`
- `tests/test_raft_snapshot_restart.cpp`
- `tests/test_raft_snapshot_diagnosis.cpp`
- `tests/snapshot_test.cpp`

是否阻塞最终验收：

- 是。

### 第六层：full persistence / restart recovery 测试

测试目标：

- 验证集群级 restart、follower catch-up、snapshot 与 committed tail replay 组合场景。
- 覆盖 Phase 2-5 的组合边界，而不是单个 storage helper。

推荐命令：

```bash
./build/tests/persistence_test
CTEST_PARALLEL_LEVEL=1 ./test.sh --group persistence
```

通过标准：

- persistence 相关测试全部通过。
- recovery 后 `commit_index`、`last_applied`、snapshot index / term、last log index 没有不一致。

失败先看：

- `modules/raft/node/raft_node.cpp`
- `modules/raft/storage/raft_storage.cpp`
- `modules/raft/storage/snapshot_storage.cpp`
- `tests/persistence_test.cpp`

是否阻塞最终验收：

- 是。

### 第七层：failure injection / crash-like 测试

测试目标：

- 验证当前已实现的 crash-like disk artifact 场景。
- 明确这不是精确 syscall failure injection，也不是真实 power-loss 证明。

推荐命令：

```bash
./build/tests/test_raft_segment_storage --gtest_filter='RaftSegmentStorageTest.TruncatesCorruptedSegmentTailDuringRecovery:RaftSegmentStorageTest.TruncatesPartialSegmentHeaderDuringRecovery:RaftSegmentStorageTest.CorruptedEarlierSegmentTailCleansLaterSegmentsAndReportsDiagnostics:RaftSegmentStorageTest.RecoveryIgnoresTemporaryPublishArtifacts:RaftSegmentStorageTest.MetaAndLogPublishWindowUsesOnlyTrustedBoundary'
./build/tests/test_snapshot_storage_reliability --gtest_filter='SnapshotStorageReliabilityTest.IgnoresStagingAndIncompleteSnapshotDirectories:SnapshotStorageReliabilityTest.ReportsValidationIssuesForSkippedSnapshotEntries:SnapshotStorageReliabilityTest.FallsBackToOlderSnapshotWhenNewestIsCorrupted:SnapshotStorageReliabilityTest.AllInvalidSnapshotsReturnNoTrustedSnapshotWithDiagnostics'
```

通过标准：

- 坏 tail、temp publish artifact、crossed meta/log、invalid snapshot catalog 都按 trusted-state contract 恢复或拒绝。

失败先看：

- `modules/raft/storage/raft_storage.cpp`
- `modules/raft/storage/snapshot_storage.cpp`
- `tests/test_raft_segment_storage.cpp`
- `tests/test_snapshot_storage_reliability.cpp`

是否阻塞最终验收：

- 是，针对当前 crash-like 覆盖范围。
- 不覆盖 T613-T616；这些是后续 Phase 6C 风险项。

### 第八层：完整 CTest / `test.sh` 总回归

测试目标：

- 确认 persistence reliability 变更没有破坏其它 Raft、KV、service、runtime 测试。
- 作为最终合入前的全量门禁。

推荐命令：

```bash
CTEST_PARALLEL_LEVEL=1 ctest --test-dir build --output-on-failure
CTEST_PARALLEL_LEVEL=1 ./test.sh --group all
```

通过标准：

- 全量测试通过。
- 没有新增跳过、删除、弱化测试的行为。

失败先看：

- 先看失败测试输出和对应测试文件。
- 如果失败涉及持久化或恢复，再回看 `modules/raft/storage/*`、`modules/raft/node/raft_node.cpp`、snapshot 相关测试。

是否阻塞最终验收：

- 是。

## 6. 每类测试的推荐命令汇总

```bash
cmake --preset debug-ninja-low-parallel
cmake --build --preset debug-ninja-low-parallel
./build/tests/test_raft_segment_storage
./build/tests/persistence_test
./build/tests/test_snapshot_storage_reliability
./build/tests/test_raft_snapshot_restart
./build/tests/test_raft_snapshot_diagnosis
./build/tests/snapshot_test --gtest_filter='RaftSnapshotRecoveryTest.SavesSnapshotAndRestoresAfterRestart'
CTEST_PARALLEL_LEVEL=1 ./test.sh --group persistence
CTEST_PARALLEL_LEVEL=1 ctest --test-dir build --output-on-failure
CTEST_PARALLEL_LEVEL=1 ./test.sh --group all
```

如果需要保留失败现场，使用：

```bash
./test.sh --keep-data --group persistence
```

## 7. 最终验收标准

- 所有相关 targeted 测试通过。
- 完整 `ctest` 或 `./test.sh --group all` 通过。
- 没有删除、跳过或弱化旧测试。
- 没有修改 `meta.bin`、segment log、snapshot data、snapshot metadata 的持久化格式。
- 没有修改 proto / RPC / KV / transport / dynamic membership 语义。
- Phase 2-6 的 phase report 都记录了测试结果。
- Windows 未实机或 CI 验证时，不能声称跨平台已完全验证。
- power loss 只能描述为 crash-like artifact 近似覆盖，不能描述为真实断电验证。
- T613-T616 等 deferred failure injection 项必须保留风险等级和后续处理建议。
- 所有剩余问题都有风险等级、影响范围和建议处理方式。
- 若最终发布文档需要代表当前实现，`docs/PERSISTENCE_DURABILITY_CONTRACT.md` 应补充 Phase 2-6 后的最终状态说明。

## 8. 是否建议进入最终测试执行

建议进入最终测试执行。

当前 targeted 阶段测试证据覆盖 segment log、meta hard state、snapshot staged publish、restart recovery diagnostics 和 crash-like artifact 场景。进入最终测试前不需要新增实现阶段，但应按本文顺序运行完整回归，并明确记录当前平台、文件系统和命令结果。

## 9. 是否建议提交最终 persistence reliability 收尾 commit

暂不建议在未执行完整总回归前提交最终收尾 commit。

建议顺序：

1. 先按本文执行最终测试。
2. 若全部通过，补齐最终测试结果记录。
3. 若需要最终文档代表当前实现，更新 `docs/PERSISTENCE_DURABILITY_CONTRACT.md` 的 Phase 2-6 后状态。
4. 再提交 persistence reliability 收尾 commit。

## 10. 明确确认没有修改业务代码

本次仅新增最终验收分析与测试方案文档。未修改 `.h` / `.cpp`，未新增测试代码，未修 bug，未修改持久化格式，未修改 Raft 协议，未修改 KV，未删除测试，也未创建 task 子目录。
