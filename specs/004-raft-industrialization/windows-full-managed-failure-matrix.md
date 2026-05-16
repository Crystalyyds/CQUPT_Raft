# Windows Full Managed Failure Matrix

## 目标

把 `T034` 首次 Windows full managed CTest sweep 的当前红灯整理成单一失败矩阵。
本文件是唯一允许保留完整失败测试名的位置；其他主文档只保留摘要、状态和链接。

## 结果来源

- 复用 `T033` / `T034` 已有日志：
  - `tmp/windows-release-managed-tests.log`
  - `tmp/test-ps1-managed.log`
- 本次 `T035` 没有重新运行测试。

## 当前摘要

- Linux full managed CTest：`104/104` PASS
  - `platform-neutral`：100 tests PASS
  - `durability-boundary`：4 tests PASS
- Windows conservative baseline：`PASS`
  - `ctest --preset windows-release-tests`
  - 当前仍只覆盖 `CommandTest`、`KvStateMachineTest`、`TimerSchedulerTest`、`ThreadPoolTest`
- Windows full managed：`FAIL`
  - `ctest --preset windows-release-managed-tests`
  - `.\test.ps1 -Managed`
  - 当前仍失败数量：`85`

## 失败分类矩阵

| 分类 | 当前状态 | 失败数量 | 典型信号 | 对应后续任务 |
|------|----------|----------|----------|--------------|
| Windows full managed CTest entry / harness 问题 | 已确认无独立阻塞 | 0 | `104` 个受管测试都已 discover 并执行；preset / wrapper 不是“没跑起来” | T036 |
| Windows runtime / timing / harness 问题 | 已收紧当前路径假设；无独立剩余项 | 0 | `raft_integration_test.cpp` 已改用更短的 Windows 临时根路径；`create temp log dir failed` 不再是当前独立 blocker | T037 |
| Windows election / replication / commit-apply 红灯 | FAIL | 17 | election / replication / commit-apply 断言失败，且当前不能写成 Windows 已收口 | T038 |
| Windows snapshot / restart / catch-up 红灯 | FAIL | 18 | snapshot restore、restart replay、catch-up sequencing、diagnosis 断言失败 | T039 |
| Windows persistence / segment / storage 红灯 | FAIL | 34 | trusted-state、meta/log publish、segment recovery、snapshot catalog/staging 断言失败 | T040 |
| Windows durability semantics adapt-or-defer | FAIL | 16 | exact seam：`FlushFileBuffers`、directory sync、replace / rename、partial write、prune / remove；当前还会阻塞部分 cluster-style 选主前持久化 | T041 |
| 其他 / 待进一步分类 | 当前无独立项 | 0 | 当前 `85` 项都能先落到上面几类 | T036 / T037 |

## 受管目标状态矩阵

| 受管目标 | 状态 | 失败数量 | 主要分类 | 后续任务 |
|----------|------|----------|----------|----------|
| `test_command` | PASS | 0 | N/A | N/A |
| `test_state_machine` | PASS | 0 | N/A | N/A |
| `test_min_heap_timer` | PASS | 0 | N/A | N/A |
| `test_thread_pool` | PASS | 0 | N/A | N/A |
| `test_kv_service` | FAIL | 2 | Windows durability semantics adapt-or-defer | T041 |
| `test_raft_election` | FAIL | 2 | Windows election / replication / commit-apply 红灯 | T038 |
| `test_raft_log_replication` | FAIL | 2 | Windows election / replication / commit-apply 红灯 | T038 |
| `test_raft_commit_apply` | FAIL | 2 | Windows election / replication / commit-apply 红灯 | T038 |
| `test_raft_split_brain` | FAIL | 7 | Windows election / replication / commit-apply 红灯 | T038 |
| `test_t017_leader_switch_ordering` | FAIL | 2 | Windows election / replication / commit-apply 红灯 | T038 |
| `persistence_test` | FAIL | 12 | Windows persistence / segment / storage 红灯 | T040 |
| `snapshot_test` | FAIL | 7 | Windows snapshot / restart / catch-up 红灯 | T039 |
| `raft_integration_test` | FAIL | 5 | Windows durability semantics adapt-or-defer | T041 |
| `test_raft_snapshot_catchup` | FAIL | 4 | Windows snapshot / restart / catch-up 红灯 | T039 |
| `test_raft_snapshot_restart` | FAIL | 4 | Windows snapshot / restart / catch-up 红灯 | T039 |
| `test_raft_snapshot_diagnosis` | FAIL | 4 | Windows snapshot / restart / catch-up 红灯 | T039 |
| `test_raft_segment_storage` | FAIL | 19 | Windows persistence / segment / storage 红灯 | T040 |
| `test_snapshot_storage_reliability` | FAIL | 11 | Windows persistence / segment / storage 红灯 | T040 |
| `test_raft_replicator_behavior` | FAIL | 2 | Windows election / replication / commit-apply 红灯 | T038 |

## 当前失败详情

以下只保留当前仍失败的测试名；已经通过的 `CommandTest`、`KvStateMachineTest`、
`TimerSchedulerTest`、`ThreadPoolTest` 不再出现在失败矩阵中。

### 1. Windows full managed CTest entry / harness 问题

当前没有独立失败项落入这一类。现有证据更支持“入口可用，但跑出真实红灯”：

- `ctest --preset windows-release-managed-tests` 能执行完整 `104` 个受管测试
- `.\test.ps1 -Managed` 复现同一组 `85` 个失败

T036 结论：

- 当前可记为 `confirmed no entry blocker / no-op`
- 现有红灯继续转交 `T037-T041`

### 2. Windows runtime / timing / harness 问题

`T037` 当前没有独立剩余失败项。

本轮已确认的结论：

- `tests/raft_integration_test.cpp` 已在 Windows 下改用更短的临时测试根路径，
  原先的 `create temp log dir failed: 文件名或扩展名太长` 信号不再出现。
- 重新跑 `RaftIntegrationTest.*` 后，5 个测试仍然失败，但主信号已经统一变成
  `FlushFileBuffers ... GetLastError=5`，不再属于纯路径/临时目录 blocker。
- `RaftKvServiceTest.*` 重新跑后也呈现相同的 `FlushFileBuffers ... GetLastError=5`
  主信号。

因此，原先暂放在 `T037` 的 7 个失败已转交 `T041`，作为 Windows durability
semantics adapt-or-defer 问题继续处理，而不是继续停留在 runtime/harness 桶里。

### 3. Windows election / replication / commit-apply 红灯

这些失败优先进入 `T038`，处理平台无关 Raft election / replication /
commit-apply 逻辑在 Windows full managed sweep 下的红灯：

- `RaftElectionTest.ThreeNodeClusterElectsExactlyOneLeader`
- `RaftElectionTest.FollowerRejectsClientProposeAfterLeaderIsElected`
- `RaftLogReplicationTest.LeaderProposeReplicatesLogToAllNodes`
- `RaftLogReplicationTest.MultipleSequentialEntriesStayConsistentAcrossCluster`
- `RaftCommitApplyTest.CommitAndApplyIndexesAdvanceAfterSuccessfulPropose`
- `RaftCommitApplyTest.DeleteCommandIsAppliedToAllNodes`
- `RaftSplitBrainTest.MinorityLeaderTimesOutAndDoesNotApplyUncommittedCommand`
- `RaftSplitBrainTest.LeaderStepsDownWhenHigherTermAppendEntriesArrives`
- `RaftSplitBrainTest.StaleAppendEntriesIsRejectedAfterNodeObservesHigherTerm`
- `RaftSplitBrainTest.SameTermSecondCandidateVoteIsRejected`
- `RaftSplitBrainTest.StaleCandidateVoteRequestIsRejectedEvenWithHigherTerm`
- `RaftSplitBrainTest.AppendEntriesRejectionIncludesFastBacktrackHint`
- `RaftSplitBrainTest.InstallSnapshotDiscardsSuffixWhenBoundaryTermDiffers`
- `RaftLeaderSwitchOrderingTest.CommittedStateSurvivesLeaderSwitchAndNewLeaderContinuesReplication`
- `RaftLeaderSwitchOrderingTest.LaggingFollowerCatchesUpDuringLeaderSwitchWithoutCommitApplyReordering`
- `RaftReplicatorBehaviorTest.SlowFollowerDoesNotBlockMajorityCommit`
- `RaftReplicatorBehaviorTest.SlowFollowerCatchesUpWhileLeaderKeepsAcceptingNewLogs`

### 4. Windows snapshot / restart / catch-up 红灯

这些失败优先进入 `T039`，处理 snapshot、restart、catch-up、diagnosis 相关
红灯：

- `RaftSnapshotRecoveryTest.SavesSnapshotAndRestoresAfterRestart`
- `RaftSnapshotRecoveryTest.FullRestartReplaysSnapshotTailWithoutLosingDeletesOrOverwrites`
- `RaftSnapshotRecoveryTest.RestartedFollowerAppliesCommittedTailExactlyOnceAfterSnapshotLoad`
- `RaftSnapshotRecoveryTest.StandaloneRestartFallsBackToOlderTrustedSnapshotWhenNewestSnapshotIsCorrupted`
- `RaftSnapshotRecoveryTest.StandaloneRestartRejectsMetadataMismatchedVisibleSnapshotAndKeepsTrustedBoundary`
- `RaftSnapshotRecoveryTest.AllPublishedSnapshotsInvalidYieldNoTrustedSnapshot`
- `RaftSnapshotCatchupTest.RestartedFollowerCatchesUpLargeGapWithBatchedAppendEntries`
- `RaftSnapshotCatchupTest.LaggingFollowerReplaysLiveLogWithoutBreakingCommittedDeleteOrdering`
- `RaftSnapshotCatchupTest.RestartedFollowerInstallsSnapshotWhenLeaderCompactedLogs`
- `RaftSnapshotCatchupTest.FollowerContinuesReplicatingLogsAfterInstallingSnapshot`
- `RaftSnapshotRestartTest.FollowerKeepsStateAfterInstallSnapshotAndRestart`
- `RaftSnapshotRestartTest.LeaderKeepsCompactedSnapshotStateAfterRestart`
- `RaftSnapshotRestartTest.FullClusterRestartsAfterSnapshotAndContinuesWriting`
- `RaftSnapshotRestartTest.SnapshotAndPostSnapshotLogsRecoverAfterFullRestart`
- `RaftSnapshotDiagnosisTest.RestartedSingleNodeLoadsSnapshotAndTailLogsWithoutPeers`
- `RaftSnapshotDiagnosisTest.CompactedClusterReplicatesNewLogAfterRestartedLeaderStepsDown`
- `RaftSnapshotDiagnosisTest.RestartedSingleNodeReplaysAppliedTailAfterRejectingCorruptedNewestSnapshot`
- `RaftSnapshotDiagnosisTest.RestartedSingleNodeSkipsMetadataMismatchedVisibleSnapshotAndReplaysCommittedTail`

### 5. Windows persistence / segment / storage 红灯

这些失败优先进入 `T040`，处理 platform-neutral 的 persistence / segment /
storage 恢复与路径问题：

- `PersistenceTest.FullClusterRestartRecovery`
- `PersistenceTest.RestartedFollowerCatchesUp`
- `PersistenceTest.ColdRestartPreservesPersistedHardStateBeforeStart`
- `PersistenceTest.ColdRestartClampsCommitAndApplyBoundariesToLastLogIndex`
- `PersistenceTest.ColdRestartUsesPreviouslyTrustedMetaBoundaryWhenNewLogPublishesBeforeMeta`
- `PersistenceTest.ColdRestartClampsCommitIndexToLastLogAndReplaysCommittedPrefix`
- `PersistenceTest.ColdRestartClampsLastAppliedToCommitIndexWhenAppliedExceedsCommit`
- `PersistenceTest.ColdRestartClampsLastAppliedToTrustedLogPrefixWhenAppliedPointsPastAvailableLog`
- `PersistenceTest.ColdRestartUsesOlderMetaTermAndVoteWhenNewerLogTreeIsVisible`
- `PersistenceTest.NewMetaWithOldLogBoundaryRejectsUntrustedCurrentTermAndVote`
- `RaftSegmentStorageTest.WritesMultipleSegmentFilesUnderBuildDirectory`
- `RaftSegmentStorageTest.AutomaticallyDeletesObsoleteSegmentsAfterCompactionSave`
- `RaftSegmentStorageTest.TruncatesCorruptedSegmentTailDuringRecovery`
- `RaftSegmentStorageTest.TruncatesPartialSegmentHeaderDuringRecovery`
- `RaftSegmentStorageTest.SaveDoesNotLeaveTemporaryOrBackupLogDirectories`
- `RaftSegmentStorageTest.SavePublishesMetaFileWithoutLeavingMetaTempFile`
- `RaftSegmentStorageTest.RecoveryIgnoresTemporaryPublishArtifacts`
- `RaftSegmentStorageTest.MetaAndLogPublishWindowUsesOnlyTrustedBoundary`
- `RaftSegmentStorageTest.MissingFirstSegmentFailsBeforeTrustingPublishedBoundary`
- `RaftSegmentStorageTest.FinalSegmentTailTruncateKeepsTrustedLogPrefixAndClampsCommitApply`
- `RaftSegmentStorageTest.MissingMetaFileCausesLoadToReportNoPersistedState`
- `RaftSegmentStorageTest.CorruptedMetaFileFailsLoad`
- `RaftSegmentStorageTest.UnsupportedMetaVersionFailsLoadWithPathAndVersion`
- `RaftSegmentStorageTest.InconsistentMetaLogBoundaryFailsBeforeTrustingSegments`
- `RaftSegmentStorageTest.CorruptedEarlierSegmentTailCleansLaterSegmentsAndReportsDiagnostics`
- `RaftSegmentStorageTest.RaftClusterGeneratesManySnapshotsAndSegmentLogsUnderBuildDirectory`
- `SnapshotStorageReliabilityTest.SavesSnapshotAsDirectoryWithDataAndMeta`
- `SnapshotStorageReliabilityTest.PublishesSnapshotWithCompatibleFinalLayout`
- `SnapshotStorageReliabilityTest.IgnoresStagingAndIncompleteSnapshotDirectories`
- `SnapshotStorageReliabilityTest.ReportsValidationIssuesForSkippedSnapshotEntries`
- `SnapshotStorageReliabilityTest.FallsBackToOlderSnapshotWhenNewestIsCorrupted`
- `SnapshotStorageReliabilityTest.AllInvalidSnapshotsReturnNoTrustedSnapshotWithDiagnostics`
- `SnapshotStorageReliabilityTest.SameIndexSameTermSaveIsIdempotent`
- `SnapshotStorageReliabilityTest.PrunesOldSnapshotDirectoriesByIndex`

### 6. Windows durability semantics adapt-or-defer

这些 exact seam 用例，以及当前被 `FlushFileBuffers ... GetLastError=5` 阻塞的
cluster-style 运行时失败，优先进入 `T041`。它们不能被写成“Windows 已等价验证
Linux-specific durability / failure-injection”：

- `RaftKvServiceTest.SingleNodeSupportsPutGetDeleteAndStatusHealth`
- `RaftKvServiceTest.ThreeNodeFollowerRedirectsWritesAndReadsToLeader`
- `RaftIntegrationTest.ElectsSingleLeaderInThreeNodeCluster`
- `RaftIntegrationTest.ReplicatesSetAndDeleteCommandsToAllNodes`
- `RaftIntegrationTest.ElectsNewLeaderAfterCurrentLeaderStops`
- `RaftIntegrationTest.GeneratesSnapshotMetaFileAfterEnoughAppliedLogs`
- `RaftIntegrationTest.LaggingFollowerInstallsSnapshotAndReplaysTailDeleteAcrossCompactionBoundary`
- `PersistenceTest.MetaFileSyncFailureNeedsExactFailureInjectionSeam`
- `PersistenceTest.MetaDirectorySyncFailureNeedsExactFailureInjectionSeam`
- `RaftSnapshotRecoveryTest.RestartAfterSnapshotPublishFailureNeedsExactFailureInjectionSeam`
- `RaftSegmentStorageTest.LogDirectoryReplaceFailureNeedsExactFailureInjectionSeam`
- `RaftSegmentStorageTest.LogDirectorySyncFailureNeedsExactFailureInjectionSeam`
- `RaftSegmentStorageTest.FinalSegmentPartialWriteNeedsExactFailureInjectionSeam`
- `SnapshotStorageReliabilityTest.StagedSnapshotPublishFailureNeedsExactFailureInjectionSeam`
- `SnapshotStorageReliabilityTest.SnapshotDirectorySyncFailureNeedsExactFailureInjectionSeam`
- `SnapshotStorageReliabilityTest.SnapshotPruneRemoveFailureNeedsExactFailureInjectionSeam`

### 7. 其他 / 待进一步分类

当前没有独立失败项落入这一类。若后续 `T036` / `T037` 发现某些失败实际上属于
preset、discover、working directory、multi-config、output directory 或 test
filter 问题，再从本矩阵迁出并重分配。
