# 003-persistence-reliability Tasks

## Phase 1 History

- [ ] T001 阅读 storage / snapshot / node 相关 AGENTS.md
- [ ] T002 梳理 meta.bin 写入和恢复路径 `moved-to-Phase-3`
- [ ] T003 梳理 log segment append / truncate / recovery 路径
- [ ] T004 梳理 snapshot save / publish / load 路径
- [ ] T005 标记 fsync / directory fsync 缺口 `meta.bin / hard state portion moved-to-Phase-3`
- [ ] T006 标记 publish ordering 风险 `meta.bin / hard state portion moved-to-Phase-3`
- [ ] T007 编写 durability contract
- [ ] T008 规划后续测试场景
- [ ] T009 生成 Phase 1 报告

## Phase 2 Checklist

- [x] T201 明确 segment log append durability 边界
- [x] T202 明确 segment log truncate durability 边界
- [x] T203 明确 `log/` directory publish 边界
- [x] T204 规划 `WriteSegments` 受影响点
- [x] T205 规划 `ReplaceDirectory` 受影响点
- [x] T206 设计 segment log append / truncate / recovery 测试

## Phase 3 Checklist

- [x] T301 承接 T002，明确 `meta.bin` 写入与恢复路径
- [x] T302 承接 T005，明确 hard state file `fsync` / directory `fsync` 需求
- [x] T303 承接 T006，明确 `meta.bin` publish ordering 风险
- [x] T304 规划 `WriteMeta` 受影响点
- [x] T305 规划 `ReplaceFile` 受影响点
- [x] T306 规划 `PersistStateLocked` 与 hard state durable publish 边界
- [x] T307 完成 Phase 3A affected-files plan 与测试设计报告

## Phase 3B Checklist

- [x] T308 补充或更新 meta.bin / hard state restart recovery 相关测试
- [x] T309 为 `WriteMeta` 增加 file fsync 语义
- [x] T310 为 `ReplaceFile` 增加 meta.bin publish / replace 后的 directory fsync 语义
- [x] T311 检查 `PersistStateLocked` 是否正确处理并传播 `WriteMeta` / `ReplaceFile` 失败
- [x] T312 确认 POSIX 分支使用真实 fsync
- [x] T313 确认 Windows 分支不使用 no-op success，必要时使用 `FlushFileBuffers` 或返回明确错误
- [x] T314 确认不修改 `meta.bin` 持久化格式
- [x] T315 运行 persistence / restart recovery 相关测试
- [x] T316 更新 `progress.md`
- [x] T317 更新 `decisions.md`
- [x] T318 生成 `specs/003-persistence-reliability/phase-reports/phase-3-meta-hard-state-fsync.md`

## Phase 4A Checklist

- [x] T401 梳理本地 snapshot save / publish 调用链
- [x] T402 梳理 `InstallSnapshot` 持久化 / 加载 / 恢复调用链
- [x] T403 梳理 startup snapshot load / trusted-state 选择规则
- [x] T404 梳理 snapshot data / metadata 与 state machine work-file 边界
- [x] T405 标记 snapshot file `fsync` / directory `fsync` 缺口
- [x] T406 标记 snapshot publish crash window 与 log compaction 边界
- [x] T407 规划 Phase 4B 受影响文件与测试范围
- [x] T408 生成 `specs/003-persistence-reliability/phase-reports/phase-4a-snapshot-atomic-publish-plan.md`

## Phase 4B Checklist

- [x] T409 将 snapshot publish 从 direct-to-final-dir 改为 staged temp snapshot dir publish
- [x] T410 为 `snapshot_<index>/data.bin` 增加 file fsync 语义
- [x] T411 为 `snapshot_<index>/__raft_snapshot_meta` 增加 file fsync 语义
- [x] T412 为 staged snapshot dir 和 `snapshot_dir` publish / prune 路径补齐 directory fsync 语义
- [x] T413 处理同 index snapshot 覆盖路径，消除 `remove_all(final_dir)` 先删后发的 crash window
- [x] T414 验证 `SnapshotWorkerLoop` / `OnInstallSnapshot` 与 snapshot publish 的边界；未发现必须修改 `raft_node.cpp` 的失败暴露或时序缺口
- [x] T415 确认 restart recovery 继续忽略 temp / incomplete snapshot，并保持“最新有效优先、无效回退到旧 snapshot”
- [x] T416 更新 `tests/test_snapshot_storage_reliability.cpp`，覆盖 staged publish、缺失 meta、缺失 data、损坏 checksum 和 temp dir 忽略场景
- [x] T417 检查 `tests/snapshot_test.cpp`、`tests/test_raft_snapshot_restart.cpp`、`tests/test_raft_snapshot_diagnosis.cpp` 中与 snapshot publish / restart recovery 直接相关的断言；现有断言仍覆盖最终布局和恢复行为，无需源码修改
- [x] T418 确认 POSIX 分支使用真实 fsync，Windows 分支不允许 no-op success；若目录 flush 无法等价实现则返回明确错误
- [x] T419 确认不修改 snapshot data / metadata 持久化格式
- [x] T420 运行 snapshot / restart / recovery 相关测试
- [x] T421 更新 `progress.md`
- [x] T422 更新 `decisions.md`
- [x] T423 生成 `specs/003-persistence-reliability/phase-reports/phase-4-snapshot-atomic-publish.md`

## Phase 5A Checklist

- [x] T501 梳理 restart recovery 整体调用链：`storage_->Load`、`LoadLatestSnapshotOnStartup`、`ApplyCommittedEntries`
- [x] T502 梳理 `meta.bin` 加载、校验、失败处理与诊断现状
- [x] T503 梳理 segment log 加载、校验、tail truncate、后续 segment 清理与诊断现状
- [x] T504 梳理 snapshot 枚举、校验、选择、加载、skip/fallback 与诊断现状
- [x] T505 梳理 meta / log / snapshot 之间的 trusted-state 边界关系
- [x] T506 梳理 recovery 后 `commit_index`、`last_applied`、`last_snapshot_index`、`last_snapshot_term` 的设置与一致性风险
- [x] T507 梳理 restart recovery 相关测试覆盖与缺口
- [x] T508 规划 Phase 5B 受影响文件与测试范围
- [x] T509 更新 `progress.md` 与 `decisions.md`
- [x] T510 生成 `specs/003-persistence-reliability/phase-reports/phase-5a-restart-recovery-diagnostics-plan.md`

## Phase 5B Checklist

- [x] T511 为 `ReadMeta` / `LoadSegmented` / `LoadSegments` 增加 path、字段名、meta boundary 与 segment boundary 诊断上下文
- [x] T512 明确并测试 `meta.bin` 的 log boundary invariant：`log_count`、`first_log_index`、`last_log_index` 与实际 segment 内容必须一致
- [x] T513 为 segment tail corruption truncate 增加诊断：原因、segment path、truncate offset、保留记录数与被清理的后续 segment
- [x] T514 为 snapshot catalog validation 暴露 skip reason，覆盖缺失 meta、缺失 data、checksum mismatch、temp/staging 目录忽略等场景
- [x] T515 为 startup snapshot recovery 增加诊断：候选数量、被跳过 snapshot、最终选择 snapshot、无可用 snapshot 或全部无效 snapshot 的结果
- [x] T516 增加 recovery 后状态摘要与一致性校验诊断：`commit_index`、`last_applied`、`last_snapshot_index`、`last_snapshot_term`、last log index、replay range
- [x] T517 补充 meta 缺失、损坏、unsupported version、boundary 不一致、commit/applied 越界 clamp 的 restart recovery 测试
- [x] T518 补充 snapshot 缺失、损坏、不完整、temp 目录残留、全部 invalid、最新 invalid 回退旧 valid 的诊断测试
- [x] T519 补充 segment tail truncate 诊断与后续 segment 清理测试
- [x] T520 检查并稳定 snapshot restart recovery 相关测试，不删除、不跳过失败测试
- [x] T521 运行 persistence / segment storage / snapshot restart / snapshot diagnosis 相关测试
- [x] T522 更新 `progress.md`
- [x] T523 更新 `decisions.md`
- [x] T524 生成 `specs/003-persistence-reliability/phase-reports/phase-5-restart-recovery-diagnostics.md`

## Phase 6A Checklist

- [x] T601 分析当前 persistence / snapshot / restart recovery 测试覆盖
- [x] T602 识别 `meta.bin`、segment log、snapshot publish 和 restart recovery 的关键 crash window
- [x] T603 区分可通过坏文件 / 坏目录构造完成的测试场景
- [x] T604 区分必须依赖 test-only failure injection 的测试场景
- [x] T605 识别 fsync、directory fsync、rename / replace、remove / prune、partial write 失败模拟缺口
- [x] T606 判断是否需要新增 crash matrix 文档或章节
- [x] T607 规划 Phase 6B 最小测试实现范围
- [x] T608 更新 `progress.md` 与 `decisions.md`
- [x] T609 生成 `specs/003-persistence-reliability/phase-reports/phase-6a-crash-failure-injection-plan.md`

## Phase 6B Checklist

- [x] T610 新增或更新 crash matrix 文档，映射对象、操作、crash point、预期恢复行为、测试方式和对应测试文件 `covered-in-phase-report`
- [x] T611 补充文件 / 目录构造类测试：残留 `meta.bin.tmp`、`log.tmp/`、`log.bak/`、旧 meta + 新 log、新 meta + 旧 log、缺失 segment、额外 segment `partial: no new missing/extra segment test because existing boundary/count tests already cover invalid segment sets`
- [x] T612 补充 snapshot 文件 / 目录构造类测试：残留 `.snapshot_staging_*`、缺失 data、缺失 meta、checksum mismatch、全部 invalid snapshot、最新 invalid 回退旧 valid
- [ ] T613 设计最小 test-only failure injection helper，用于 storage 持久化路径中的 sync、rename / replace、remove / prune、write / copy failure `deferred: not introduced in Phase 6B because it requires production hook surface`
- [ ] T614 补充 segment log failure injection 测试：`WriteSegments` file sync 失败、`ReplaceDirectory` publish / cleanup / directory sync 失败、失败后重启只接受可信 `log/` `deferred: exact fsync / rename failure needs test-only hook`
- [ ] T615 补充 `meta.bin` failure injection 测试：`WriteMeta` file sync 失败、`ReplaceFile` publish / directory sync 失败、失败后 restart recovery 不信任部分发布状态 `deferred: exact fsync / rename failure needs test-only hook`
- [ ] T616 补充 snapshot failure injection 测试：staging data/meta sync 失败、staging dir sync 失败、publish rename 失败、parent directory sync 失败、prune 删除 / sync 失败 `deferred: exact sync / rename / prune failure needs test-only hook`
- [x] T617 补充 restart recovery crash-point 测试：验证每个 crash window 后只能恢复旧完整状态或新完整状态，不能接受部分发布对象
- [x] T618 确认 failure injection 默认关闭，不改变生产路径语义、持久化格式、Raft 协议、KV 逻辑或公共 API 行为
- [x] T619 运行 persistence / segment storage / snapshot reliability / snapshot restart / snapshot diagnosis 相关测试
- [x] T620 更新 `progress.md`
- [x] T621 更新 `decisions.md`
- [x] T622 生成 `specs/003-persistence-reliability/phase-reports/phase-6-crash-failure-injection-tests.md`

## Final Linux Validation Checklist

- [ ] T701 Linux clean build and基础单测验证
  - 测试目标：确认 Linux 下 clean configure / build / link 成功，且基础单测不受 persistence reliability 变更影响。
  - 推荐命令：`cmake --preset debug-ninja-low-parallel && cmake --build --preset debug-ninja-low-parallel && ctest --test-dir build --output-on-failure -R "Command|StateMachine|Timer|Thread|Config"`
  - 通过标准：configure、build、link 全部成功；基础单测通过；没有新增跳过或删除测试。
  - 失败时优先查看：`CMakeLists.txt`、`tests/CMakeLists.txt`、失败测试对应模块。
  - 是否阻塞最终验收：是。

- [ ] T702 Linux storage / segment log durability 与 recovery 测试
  - 测试目标：覆盖 segment log append、truncate、recovery、tail corruption truncate、checksum / partial header、meta/log boundary 与 `log.tmp` / `log.bak` 残留恢复。
  - 推荐命令：`./build/tests/test_raft_segment_storage`
  - 通过标准：`RaftSegmentStorageTest` 全部通过，尤其是 tail corruption、partial segment header、temporary publish artifact、old meta + new log、新 meta + old log 相关用例通过。
  - 失败时优先查看：`modules/raft/storage/raft_storage.cpp`、`tests/test_raft_segment_storage.cpp`。
  - 是否阻塞最终验收：是。

- [ ] T703 Linux POSIX/fsync 路径覆盖确认
  - 测试目标：确认 Linux 运行的 storage、meta、snapshot 测试实际覆盖 POSIX `fsync` / directory `fsync` 路径，不把 Windows `FlushFileBuffers` 视为已验证。
  - 推荐命令：`./build/tests/test_raft_segment_storage && ./build/tests/persistence_test && ./build/tests/test_snapshot_storage_reliability`
  - 通过标准：相关测试在 Linux 通过；最终记录明确写出 POSIX 路径已验证、Windows 路径未实机验证。
  - 失败时优先查看：`modules/raft/storage/raft_storage.cpp`、`modules/raft/storage/snapshot_storage.cpp`。
  - 是否阻塞最终验收：是，针对 Linux-only 验收。

- [ ] T704 Linux meta.bin / hard state persistence 测试
  - 测试目标：覆盖 `meta.bin` 缺失、损坏、unsupported version、`term` / `vote` / `commit_index` / `last_applied` recovery，以及 meta boundary 与 segment log 内容不一致。
  - 推荐命令：`./build/tests/persistence_test --gtest_filter='PersistenceTest.ColdRestartPreservesPersistedHardStateBeforeStart:PersistenceTest.ColdRestartClampsCommitAndApplyBoundariesToLastLogIndex' && ./build/tests/test_raft_segment_storage --gtest_filter='RaftSegmentStorageTest.SavePublishesMetaFileWithoutLeavingMetaTempFile:RaftSegmentStorageTest.MissingMetaFileCausesLoadToReportNoPersistedState:RaftSegmentStorageTest.CorruptedMetaFileFailsLoad:RaftSegmentStorageTest.UnsupportedMetaVersionFailsLoadWithPathAndVersion:RaftSegmentStorageTest.InconsistentMetaLogBoundaryFailsBeforeTrustingSegments'`
  - 通过标准：hard state restart 值符合预期；损坏或不一致的 meta/log 状态不会被接受为 trusted state。
  - 失败时优先查看：`modules/raft/storage/raft_storage.cpp`、`modules/raft/node/raft_node.cpp`、`tests/persistence_test.cpp`、`tests/test_raft_segment_storage.cpp`。
  - 是否阻塞最终验收：是。

- [ ] T705 Linux snapshot storage reliability 测试
  - 测试目标：覆盖 staged snapshot publish、缺失 `data.bin`、缺失 `__raft_snapshot_meta`、checksum mismatch、temp / staging snapshot 目录残留、最新 invalid snapshot 回退旧 valid snapshot。
  - 推荐命令：`./build/tests/test_snapshot_storage_reliability`
  - 通过标准：`SnapshotStorageReliabilityTest` 全部通过；invalid / incomplete snapshot 不被 catalog 视为 trusted snapshot。
  - 失败时优先查看：`modules/raft/storage/snapshot_storage.cpp`、`modules/raft/storage/snapshot_storage.h`、`tests/test_snapshot_storage_reliability.cpp`。
  - 是否阻塞最终验收：是。

- [ ] T706 Linux restart recovery / diagnosis 测试
  - 测试目标：覆盖 snapshot restart、snapshot diagnosis、post-snapshot tail log replay，以及 recovery 后 `commit_index`、`last_applied`、snapshot index / term、last log index 一致性。
  - 推荐命令：`./build/tests/test_raft_snapshot_restart && ./build/tests/test_raft_snapshot_diagnosis && ./build/tests/snapshot_test --gtest_filter='RaftSnapshotRecoveryTest.SavesSnapshotAndRestoresAfterRestart'`
  - 通过标准：snapshot restart / diagnosis 测试通过；恢复诊断能解释 skip reason、selected snapshot 和 post-state summary。
  - 失败时优先查看：`modules/raft/node/raft_node.cpp`、`modules/raft/storage/snapshot_storage.cpp`、`tests/test_raft_snapshot_restart.cpp`、`tests/test_raft_snapshot_diagnosis.cpp`、`tests/snapshot_test.cpp`。
  - 是否阻塞最终验收：是。

- [ ] T707 Linux crash-like / power-loss 近似测试
  - 测试目标：覆盖文件缺失、文件损坏、部分写入、temp 目录残留、boundary 不一致和 power loss 近似场景；明确这些不是精确 syscall failure injection。
  - 推荐命令：`./build/tests/test_raft_segment_storage --gtest_filter='RaftSegmentStorageTest.TruncatesCorruptedSegmentTailDuringRecovery:RaftSegmentStorageTest.TruncatesPartialSegmentHeaderDuringRecovery:RaftSegmentStorageTest.CorruptedEarlierSegmentTailCleansLaterSegmentsAndReportsDiagnostics:RaftSegmentStorageTest.RecoveryIgnoresTemporaryPublishArtifacts:RaftSegmentStorageTest.MetaAndLogPublishWindowUsesOnlyTrustedBoundary' && ./build/tests/test_snapshot_storage_reliability --gtest_filter='SnapshotStorageReliabilityTest.IgnoresStagingAndIncompleteSnapshotDirectories:SnapshotStorageReliabilityTest.ReportsValidationIssuesForSkippedSnapshotEntries:SnapshotStorageReliabilityTest.FallsBackToOlderSnapshotWhenNewestIsCorrupted:SnapshotStorageReliabilityTest.AllInvalidSnapshotsReturnNoTrustedSnapshotWithDiagnostics'`
  - 通过标准：坏 tail、temp publish artifact、crossed meta/log、invalid snapshot catalog 都按 trusted-state contract 恢复或拒绝。
  - 失败时优先查看：`modules/raft/storage/raft_storage.cpp`、`modules/raft/storage/snapshot_storage.cpp`、`tests/test_raft_segment_storage.cpp`、`tests/test_snapshot_storage_reliability.cpp`。
  - 是否阻塞最终验收：是，针对当前 Linux crash-like 覆盖范围；T613-T616 仍保留为后续 failure injection 缺口。

- [ ] T708 Linux persistence / snapshot / restart 子集回归
  - 测试目标：覆盖集群级 restart、follower catch-up、snapshot 与 committed tail replay 组合场景。
  - 推荐命令：`./build/tests/persistence_test && CTEST_PARALLEL_LEVEL=1 ./test.sh --group persistence`
  - 通过标准：persistence 相关测试全部通过；恢复后 commit/apply/snapshot/log 边界一致。
  - 失败时优先查看：`modules/raft/node/raft_node.cpp`、`modules/raft/storage/raft_storage.cpp`、`modules/raft/storage/snapshot_storage.cpp`、`tests/persistence_test.cpp`。
  - 是否阻塞最终验收：是。

- [ ] T709 Linux 完整 CTest / 项目总回归
  - 测试目标：确认 persistence reliability 变更没有破坏其它 Raft、KV、service、runtime 测试。
  - 推荐命令：`CTEST_PARALLEL_LEVEL=1 ctest --test-dir build --output-on-failure && CTEST_PARALLEL_LEVEL=1 ./test.sh --group all`
  - 通过标准：完整 CTest 或项目测试脚本全部通过；没有新增跳过、删除或弱化测试。
  - 失败时优先查看：失败测试输出和对应测试文件；若失败涉及持久化或恢复，再查看 `modules/raft/storage/*`、`modules/raft/node/raft_node.cpp`、snapshot 相关测试。
  - 是否阻塞最终验收：是。

- [ ] T710 Linux-only 验收结果记录与 Windows 未验证说明
  - 测试目标：记录本轮只执行 Linux 验收，不测试 Windows，不声明跨平台完全验证；保留 Windows durability runtime semantics 未实机验证说明。
  - 推荐命令：不执行额外命令；汇总 T701-T709 的 Linux 测试结果。
  - 通过标准：最终记录明确区分 Linux 已验证项、Windows 未验证项、power loss 近似测试与真实 power loss 未验证项。
  - 失败时优先查看：`specs/003-persistence-reliability/phase-reports/final-persistence-validation-plan.md`、`specs/003-persistence-reliability/progress.md`、`specs/003-persistence-reliability/decisions.md`。
  - 是否阻塞最终验收：是。

## Final Linux Validation Failure Resolution Checklist

- [x] T711 记录并分类当前 Linux 总验收失败
  - 目标：把已观察到的失败先归档为 final validation blocker，而不是直接改代码。
  - 已观察失败：`ctest -R "Command|StateMachine|Timer|Thread|Config"` 误匹配并首次触发 `RaftSplitBrainTest.MinorityLeaderTimesOutAndDoesNotApplyUncommittedCommand`，失败点为 `leader_index.has_value() == false`，后续完整 CTest 该用例通过。
  - 已观察失败：`CTEST_PARALLEL_LEVEL=1 ./test.sh --group all` 在 `RaftSnapshotRecoveryTest.SavesSnapshotAndRestoresAfterRestart` 失败，`propose failed at i=17, message=lost leadership before the log entry reached a majority`；该用例此前单独运行和完整 CTest 均通过。
  - 追加执行结果：`RaftSnapshotRecoveryTest.SavesSnapshotAndRestoresAfterRestart` 单独重复运行第 4 次失败，失败点为 `tests/snapshot_test.cpp:375`，`propose failed at i=3, message=lost leadership before the log entry reached a majority`。
  - 追加执行结果：`RaftSplitBrainTest.MinorityLeaderTimesOutAndDoesNotApplyUncommittedCommand` 单独通过 CTest 运行第 1 次失败，失败点为 `tests/test_raft_split_brain.cpp:296`，`leader_index.has_value() == false`。
  - 通过标准：明确区分“稳定复现失败”“时序型 flaky 失败”“测试命令误匹配导致的非目标失败”。
  - 失败时优先查看：`tests/snapshot_test.cpp`、`tests/test_raft_split_brain.cpp`、`test.sh`、`tests/CMakeLists.txt`。
  - 是否阻塞最终验收：是。

- [x] T712 复现矩阵：确认 snapshot recovery 失败触发条件
  - 目标：判断 `RaftSnapshotRecoveryTest.SavesSnapshotAndRestoresAfterRestart` 是单测本身 flaky、`test.sh --group all` 顺序相关、还是系统负载 / timing 相关。
  - 推荐命令：`for i in {1..10}; do ./build/tests/snapshot_test --gtest_filter='RaftSnapshotRecoveryTest.SavesSnapshotAndRestoresAfterRestart' || break; done`
  - 推荐命令：`for i in {1..5}; do CTEST_PARALLEL_LEVEL=1 ctest --test-dir build --output-on-failure -R 'RaftSnapshotRecoveryTest.SavesSnapshotAndRestoresAfterRestart' || break; done`
  - 推荐命令：`CTEST_PARALLEL_LEVEL=1 ./test.sh --group all`
  - 执行结果：单独重复运行 `./build/tests/snapshot_test --gtest_filter='RaftSnapshotRecoveryTest.SavesSnapshotAndRestoresAfterRestart'` 时第 4 次失败，说明该失败不是只由完整 `test.sh --group all` 顺序触发。
  - 执行结论：失败集中在 proposal loop 中的 leadership loss，当前证据指向测试时序 / leader churn 稳定性问题，需要进入 T713/T714 进一步判定和设计修复边界。
  - 通过标准：能说明失败是否只在完整脚本顺序中出现，以及失败是否集中在 proposal loop 的 leadership loss。
  - 失败时优先查看：`tests/snapshot_test.cpp` 中 propose loop、leader discovery、等待 commit/apply 的 helper。
  - 是否阻塞最终验收：是。

- [ ] T713 分析 `RaftSnapshotRecoveryTest.SavesSnapshotAndRestoresAfterRestart` 的测试假设
  - 目标：确认测试是否假设“初始 leader 在 25 条写入期间永不变更”，以及该假设是否与当前异步 Raft + snapshot fsync IO 成本冲突。
  - 检查点：proposal loop 是否在 leader 变更后继续向旧 leader写入。
  - 检查点：snapshot 生成和 durable publish 是否可能拉长 IO 时间，导致 follower election timeout。
  - 检查点：测试是否已经有 retry / re-resolve leader / wait-for-stable-leader 机制。
  - 通过标准：给出结论是“测试稳定性问题”还是“生产 Raft 心跳 / snapshot 阻塞问题”；若是生产语义问题，停止并升级范围，不在 final validation 中顺手修改业务逻辑。
  - 失败时优先查看：`tests/snapshot_test.cpp`、`modules/raft/node/raft_node.cpp` 的 snapshot worker / heartbeat / propose 相关路径。
  - 是否阻塞最终验收：是。

- [ ] T714 设计最小测试侧稳定化方案，不弱化恢复断言
  - 目标：如果 T713 证明是测试时序假设问题，则只修改测试 helper，不改变 Raft 协议、持久化格式或生产逻辑。
  - 方案要求：proposal 失败若属于 leadership loss / not leader / majority 未达成的临时状态，应重新发现当前 leader并重试 bounded 次数。
  - 方案要求：每次重试必须仍验证最终所有 key 被提交、应用、snapshot 后重启恢复，并继续写入成功。
  - 方案要求：不能通过跳过、删除、降低 command 数量或取消 snapshot 断言来通过测试。
  - 通过标准：测试对合法 leader churn 稳健，但仍能发现真正的 snapshot/restart recovery 失败。
  - 失败时优先查看：`tests/snapshot_test.cpp` 中写入循环、`WaitForLeader`、`WaitForValueOnAllNodes`、restart 后验证逻辑。
  - 是否阻塞最终验收：是。

- [ ] T715 处理 `RaftSplitBrainTest.MinorityLeaderTimesOutAndDoesNotApplyUncommittedCommand` 的误匹配与 flaky 记录
  - 目标：确认 T701 的基础单测筛选不应误包含 split-brain 集成测试，且 split-brain 用例本身是否存在独立 flaky 风险。
  - 方案选项：调整 Final Linux Validation checklist 的基础筛选正则，使其只匹配真正的基础单测。
  - 方案选项：若 split-brain 重复运行仍会失败，则单独进入稳定性修复任务，不和 persistence durability 混在一起。
  - 推荐命令：`for i in {1..10}; do ./build/tests/raft_core_tests --gtest_filter='RaftSplitBrainTest.MinorityLeaderTimesOutAndDoesNotApplyUncommittedCommand' || break; done`，实际二进制名称以构建产物为准。
  - 追加执行结果：`CTEST_PARALLEL_LEVEL=1 ctest --test-dir build --output-on-failure -R 'RaftSplitBrainTest.MinorityLeaderTimesOutAndDoesNotApplyUncommittedCommand'` 第 1 次失败，说明该用例不只是 T701 正则误匹配问题，也存在独立 leader election timing flaky 风险。
  - 通过标准：基础筛选不再产生无关失败；split-brain 若仍 flaky，有独立任务追踪。
  - 失败时优先查看：`tests/test_raft_split_brain.cpp`、`specs/003-persistence-reliability/tasks.md` 的 T701 命令。
  - 是否阻塞最终验收：是。

- [ ] T716 实施前置边界确认
  - 目标：在动手修复前确认允许修改范围，避免 final validation 修复扩大成业务逻辑变更。
  - 默认允许范围：`tests/snapshot_test.cpp`、必要的测试 helper、`specs/003-persistence-reliability/tasks.md`、`progress.md`、`decisions.md`、最终验证报告。
  - 默认禁止范围：生产 `.h` / `.cpp`、持久化格式、Raft 协议、KV、proto / RPC / transport。
  - 升级条件：若证明 snapshot fsync / snapshot worker 阻塞心跳属于生产正确性问题，则停止并单独开实现阶段，不在测试稳定化任务里修。
  - 通过标准：修复前有明确范围确认和风险分类。
  - 失败时优先查看：根 `AGENTS.md`、`modules/raft/node/AGENTS.md`、`modules/raft/storage/AGENTS.md`、相关测试文件。
  - 是否阻塞最终验收：是。

- [ ] T717 修复后稳定性验证
  - 目标：证明修复不是偶然通过。
  - 推荐命令：`for i in {1..20}; do ./build/tests/snapshot_test --gtest_filter='RaftSnapshotRecoveryTest.SavesSnapshotAndRestoresAfterRestart' || break; done`
  - 推荐命令：`CTEST_PARALLEL_LEVEL=1 ctest --test-dir build --output-on-failure -R 'RaftSnapshotRecoveryTest.SavesSnapshotAndRestoresAfterRestart|RaftSplitBrainTest.MinorityLeaderTimesOutAndDoesNotApplyUncommittedCommand'`
  - 推荐命令：`CTEST_PARALLEL_LEVEL=1 ./test.sh --group all`
  - 通过标准：重复 snapshot recovery 用例、相关 CTest filter、完整 `test.sh --group all` 全部通过。
  - 失败时优先查看：最新失败日志、`tests/snapshot_test.cpp`、`tests/test_raft_split_brain.cpp`。
  - 是否阻塞最终验收：是。

- [ ] T718 更新最终 Linux 验收状态
  - 目标：修复并验证后，把 T701-T710 的执行结果和 remaining risk 写回最终验收记录。
  - 记录要求：Linux 测试结果、曾发生的 flaky 失败、修复方式、重跑次数、仍未覆盖的 Windows / true power loss / exact failure injection 缺口。
  - 通过标准：最终记录不能声称 Windows 已验证，不能把 crash-like 测试写成真实 power loss 证明。
  - 失败时优先查看：`specs/003-persistence-reliability/phase-reports/final-persistence-validation-plan.md`、`progress.md`、`decisions.md`。
  - 是否阻塞最终验收：是。
