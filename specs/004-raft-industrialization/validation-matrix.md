# CQUPT_Raft Industrialization Validation Matrix

## Purpose

This document freezes the industrialization scope for
`004-raft-industrialization` so completed work is carried forward as protected
baseline, while only the remaining reliability, recovery, validation, and
cross-platform gaps are scheduled for follow-up work.

## Sources

- `specs/004-raft-industrialization/spec.md`
- `specs/004-raft-industrialization/plan.md`
- `specs/004-raft-industrialization/quickstart.md`
- `specs/003-persistence-reliability/progress.md`
- `test.sh`

## Scope Freeze

### Protected Baseline (Already Completed, Do Not Re-Implement)

| Area | Current status | Carry-forward evidence | Validation status | Next action |
|------|----------------|------------------------|-------------------|-------------|
| Leader election, AppendEntries, majority commit, apply progression | Implemented and already test-backed | Current `spec.md` baseline, existing election/replication/apply tests | Baseline preserved | Regression only |
| Snapshot save/load/install baseline | Implemented and already test-backed | `spec.md` baseline, snapshot/restart/catch-up tests | Baseline preserved | Regression only |
| Follower catch-up baseline | Implemented, but still needs stronger industrialization evidence | Existing catch-up and replicator tests | Baseline preserved, not reimplemented | Add targeted regression only |
| Segmented log persistence baseline | Implemented and strengthened in `003` | `003 progress.md` T201-T206 | Completed | Reuse as existing capability |
| Hard-state durability for `meta.bin` | Implemented and strengthened in `003` | `003 progress.md` T301-T318 | Completed | Reuse as existing capability |
| Snapshot atomic publish baseline | Implemented and strengthened in `003` | `003 progress.md` T401-T423 | Completed | Reuse as existing capability |
| Restart recovery diagnostics baseline | Implemented and strengthened in `003` | `003 progress.md` T501-T524 | Completed | Reuse as existing capability |
| Crash-artifact recovery coverage from malformed files/directories | Implemented in `003` without production rewrite | `003 progress.md` T601-T622 | Completed for constructed artifacts | Extend only where exact injection is still missing |

### Remaining Industrialization Work (In Scope)

| Priority | Area | Current status | Risk source | Primary validation | Linux-specific | Windows/macOS fallback |
|----------|------|----------------|-------------|--------------------|----------------|------------------------|
| P0 | Final Linux validation flaky stabilization | Not complete | `003 progress.md` blocked items for snapshot recovery and split-brain timing failures | `./test.sh --group snapshot-recovery`, `./test.sh --group election`, targeted `ctest -R` reruns | Yes | Platform-neutral rerun through `ctest --preset debug-tests`; no equivalent timing guarantee claimed |
| P0 | Exact durability failure injection (`fsync`, directory sync, rename/replace, prune/remove, partial write) | Missing | `003 progress.md` blocked T613-T616 | New targeted persistence/snapshot tests plus Linux reruns | Yes | Explicit deferred runtime validation; preserve documentation-only fallback |
| P0 | Restart trusted-state matrix for term/vote/log/snapshot metadata/applied state | US1 accepted; managed regression evidence in place | `spec.md` FR-003/FR-010, `plan.md` W3 | `./test.sh --group persistence`, `snapshot-recovery`, `diagnosis`; targeted `ctest -R '^(PersistenceTest|RaftSegmentStorageTest|RaftSnapshotRecoveryTest|RaftSnapshotDiagnosisTest)\.'` | Partially | Platform-neutral recovery tests remain valid; Linux-specific durability evidence is recorded separately and crash semantics are not over-claimed |
| P1 | Catch-up after lag, compaction, restart, and snapshot handoff | Implemented but needs stronger proof | `plan.md` W4, current gap classification `B/C` | `./test.sh --group snapshot-catchup`, `integration`, `replicator` | No | Same tests should remain runnable via CTest |
| P1 | Leader switch and commit/apply ordering under churn | Implemented but still risky | `plan.md` W4, current gap classification `B/C` | `./test.sh --group replication`, `election`, `replicator` | No | Same tests should remain runnable via CTest |
| P1 | State-machine apply/replay consistency after snapshot/restart | Implemented but needs stronger proof | `plan.md` W5 | `./test.sh --group snapshot-recovery`, targeted `ctest -R` on state machine and integration tests | No | Same tests should remain runnable via CTest |
| P2 | Unified validation entrypoints and grouped test guidance | Incomplete | Current `test.sh`, `CMakePresets.json`, `plan.md` W6 | `./test.sh --group all`, `ctest --preset debug-tests` | Partially | PowerShell or documented non-Bash fallback required |
| P3 | Failure localization and platform-support documentation | Incomplete | `plan.md` W7/W8 | Spec docs and grouped rerun commands | Partially | Explicit support matrix and follow-up notes required |
| P4 | Windows/macOS deeper runtime validation and CI expansion | Deferred follow-up | `spec.md` platform scope, `plan.md` W8 | Future runtime validation | No | This row is itself the fallback and follow-up definition |

### Explicitly Deferred (Not In Current Implementation Scope)

| Area | Why deferred |
|------|--------------|
| Protocol semantic changes | Forbidden by constitution and current task scope |
| Persisted format changes or migrations | Forbidden by constitution and current task scope |
| Public API redesign | Not required for current industrialization goals |
| Transport rewrite or snapshot streaming RPC redesign | Out of scope for this feature |
| Business storage engine expansion beyond metadata layer | Unrelated to current industrialization scope |
| Large-scale architectural refactor | Violates minimal-change constraint |

## Validation Entry Matrix

| Entrypoint | Purpose | Scope | Linux-specific | Artifact retention | Notes |
|-----------|---------|-------|----------------|--------------------|-------|
| `cmake --preset debug-ninja-low-parallel` | Primary low-parallel configure | Linux primary build path | No | No | Current preferred configure entry |
| `cmake --build --preset debug-ninja-low-parallel` | Primary low-parallel build | Linux primary build path | No | No | Current preferred build entry |
| `CTEST_PARALLEL_LEVEL=1 ./test.sh --group all` | Primary regression sweep | Linux primary validation | Partially | Optional via `--keep-data` | Includes grouped Linux execution flow |
| `./test.sh --group persistence` | Focused restart/durability rerun | Industrialization hotspot | Partially | Optional via `--keep-data` | Main trusted-state regression bucket |
| `./test.sh --group snapshot-recovery` | Focused snapshot/restart rerun | Industrialization hotspot | Partially | Optional via `--keep-data` | Current flaky blocker area |
| `./test.sh --group diagnosis` | Focused diagnostics rerun | Industrialization hotspot | Partially | Optional via `--keep-data` | Main failure-localization bucket |
| `./test.sh --group snapshot-catchup` | Focused catch-up rerun | US2 regression | No | Optional via `--keep-data` | Cluster consistency evidence |
| `./test.sh --group replicator` | Focused replicator rerun | US2 regression | No | Optional via `--keep-data` | Follower synchronization evidence |
| `./test.sh --group segment-cluster` | Focused clustered segment/snapshot stress rerun | US2/US3 regression | Partially | Optional via `--keep-data` | Main segment rollover and retained-artifact stress bucket |
| `ctest --preset debug-tests --output-on-failure` | Platform-neutral fallback | Cross-platform baseline execution contract | No | No | Must remain valid outside Bash-first flows |

## Grouped Linux Rerun Guide

Use the following groups as the primary Linux rerun buckets for failure
localization. Unless a narrower command is required, prefer
`CTEST_PARALLEL_LEVEL=1` and add `--keep-data` when restart/snapshot/segment
artifacts are needed for diagnosis.

| Group | Primary purpose | Failure classification focus | Linux primary / Linux-specific | Minimal rerun command | When to add `--keep-data` | Platform-neutral fallback |
|------|------------------|------------------------------|-------------------------------|-----------------------|---------------------------|---------------------------|
| `snapshot-recovery` | Snapshot/restart recovery path | leader churn during recovery, snapshot restore failure, restart trusted-state mismatch | Linux primary hotspot | `CTEST_PARALLEL_LEVEL=1 ./test.sh --group snapshot-recovery` | When restart artifacts, snapshot dirs, or retained node data are needed | `CTEST_PARALLEL_LEVEL=1 ctest --test-dir build --output-on-failure -R '^RaftSnapshotRecoveryTest\.'` |
| `diagnosis` | Recovery diagnostics and snapshot fallback | snapshot skip/fallback, invalid snapshot rejection, restart explanation gaps | Linux primary hotspot | `CTEST_PARALLEL_LEVEL=1 ./test.sh --group diagnosis` | When snapshot skip/fallback evidence must be retained | `CTEST_PARALLEL_LEVEL=1 ctest --test-dir build --output-on-failure -R '^RaftSnapshotDiagnosisTest\.'` |
| `snapshot-catchup` | Lagging follower catch-up and snapshot handoff | follower catch-up gap, snapshot handoff sequencing, restart after catch-up | Linux primary grouped rerun | `CTEST_PARALLEL_LEVEL=1 ./test.sh --group snapshot-catchup` | When follower catch-up state or retained snapshot/log artifacts are needed | `CTEST_PARALLEL_LEVEL=1 ctest --test-dir build --output-on-failure -R '^RaftSnapshotCatchupTest\.'` |
| `replicator` | Single follower replication state machine | replication state drift, backoff, follower catch-up behavior | Linux primary grouped rerun | `CTEST_PARALLEL_LEVEL=1 ./test.sh --group replicator` | When retained per-node data helps explain replication drift | `CTEST_PARALLEL_LEVEL=1 ctest --test-dir build --output-on-failure -R '^RaftReplicatorBehaviorTest\.'` |
| `segment-cluster` | Clustered segment/snapshot stress path | segment rollover, clustered snapshot generation, retained-artifact stress failures | Linux primary grouped rerun | `CTEST_PARALLEL_LEVEL=1 ./test.sh --group segment-cluster` | When generated segment/snapshot trees must be inspected | `CTEST_PARALLEL_LEVEL=1 ctest --test-dir build --output-on-failure -R '^RaftSegmentStorageTest\.RaftClusterGeneratesManySnapshotsAndSegmentLogsUnderBuildDirectory$'` |

Notes:

- `--keep-data` is a Linux Bash-first capability. It retains `raft_data/`,
  `raft_snapshots/`, and `build/tests/raft_test_data/` for local diagnosis.
- For Windows/macOS, the fallback entry remains
  `ctest --preset debug-tests --output-on-failure` or the corresponding direct
  `ctest --test-dir build ... -R ...` command. These fallbacks provide logic
  regression only and do not claim Linux-equivalent retained-artifact or
  crash-style runtime evidence.
- The rerun groups above are not a replacement for the full Linux primary path;
  they exist to narrow failures after a grouped run or a targeted investigation.

## Linux-Specific Boundaries

The following evidence is Linux-primary and must not be treated as completed
cross-platform runtime proof:

- Timing-sensitive validation of current flaky acceptance paths
- Exact durability failure injection around sync, directory sync, replace,
  prune, and partial-write boundaries
- Any future crash-style or process/signal-oriented failure harness
- Bash-first orchestration through `test.sh`

For these areas, Windows/macOS follow-up must either:

- use the platform-neutral `ctest --preset debug-tests` path for logic-only
  regression, or
- remain explicitly deferred until runtime validation exists.

## US1 Accepted Restart Recovery Evidence

| Evidence area | Covered scenarios | Entrypoint | Latest status | Platform scope |
|---------------|-------------------|------------|---------------|----------------|
| Hard-state and log-boundary restart matrix | old-meta/new-log, new-meta/old-log, commit/apply clamp, missing first segment, final segment tail truncate 后 trusted log prefix 选择 | `CTEST_PARALLEL_LEVEL=1 ctest --test-dir build --output-on-failure -R '^(PersistenceTest|RaftSegmentStorageTest)\.'` | PASS | 平台无关恢复逻辑证据；如涉及 exact durability 边界，则由 Linux-specific 注入测试补充 |
| Snapshot metadata and applied-state restart matrix | invalid snapshot rejection, all-invalid fallback, metadata mismatch, corrupted newest snapshot fallback, trusted snapshot 选择后一致的 applied replay | `CTEST_PARALLEL_LEVEL=1 ctest --test-dir build --output-on-failure -R '^(RaftSnapshotRecoveryTest|RaftSnapshotDiagnosisTest)\.'` | PASS | 平台无关恢复逻辑证据 |
| Linux-specific durability failure injection and diagnostics | meta/log/snapshot 的 file sync、directory sync、replace/rename、prune/remove、partial write 边界及对应 trusted-state 预期 | `CTEST_PARALLEL_LEVEL=1 ctest --test-dir build --output-on-failure -R '^(PersistenceTest|RaftSegmentStorageTest|SnapshotStorageReliabilityTest|RaftSnapshotRecoveryTest)\.'` | PASS | Linux-specific runtime evidence；Windows/macOS 仅保留逻辑回归 fallback，不声称等价注入语义 |

Current US1 status:

- T012 与 T013 的 restart matrix 已纳入受管 GTest / CTest 回归。
- T014 已修复 restart trusted-state / final-segment trusted-prefix 恢复缺口。
- 当前没有遗留 tests-first 红灯；US1 restart recovery 进入 regression-only 状态。

## US2 Accepted Consistency Evidence

| Evidence area | Covered scenarios | Entrypoint | Latest status | Platform scope |
|---------------|-------------------|------------|---------------|----------------|
| Catch-up and snapshot handoff consistency | follower 落后 live log 后通过 log replay catch-up 恢复；follower 落后到 retained snapshot boundary 后通过 snapshot handoff catch-up 恢复；catch-up 后 committed ordering 保持一致 | `CTEST_PARALLEL_LEVEL=1 ctest --test-dir build --output-on-failure -R '^(RaftSnapshotCatchupTest|RaftIntegrationTest)\.'` | PASS | 平台无关 cluster consistency 逻辑证据 |
| Leader-switch and commit/apply ordering consistency | leader 切换后 committed state 保持不变；新 leader 继续推进新日志；lagging follower、leader switch 与新 proposal 混合时 commit/apply 不逆序 | `CTEST_PARALLEL_LEVEL=1 ctest --test-dir build --output-on-failure -R '^(RaftLeaderSwitchOrderingTest)\.'` | PASS | 平台无关 cluster consistency 逻辑证据 |
| State-machine replay consistency | snapshot load 后 tail replay 不丢失；restart 后 committed log apply 一致；state machine 最终视图与 committed/applied 状态一致；duplicate apply / missed apply 风险受回归保护 | `CTEST_PARALLEL_LEVEL=1 ctest --test-dir build --output-on-failure -R '^(KvStateMachineTest|RaftSnapshotRecoveryTest)\.'` | PASS | 平台无关 replay / restart consistency 逻辑证据 |

Current US2 status:

- T016 已提供 catch-up / snapshot handoff 回归证据，当前没有暴露 `replicator.cpp` 生产缺口。
- T017 已提供 leader-switch / commit-apply ordering 回归证据，当前没有暴露 `raft_node.cpp` 生产缺口。
- T018 已提供 state-machine replay consistency 回归证据，当前没有暴露 `state_machine.cpp` 或 `raft_node.cpp` 生产缺口。
- T019、T020、T021 均以 no-op 关闭：因为对应 tests-first 回归均通过，当前没有进入生产修复的输入证据。
- 当前没有 US2 tests-first 红灯；US2 consistency 进入 regression-only 状态。
- US2 完成后，lag/restart/leader-switch 一致性已具备独立验收所需的回归证据，不依赖新增协议语义或大范围生产代码重写。

### US2 Platform-Neutral Regression Coverage

- catch-up after lag:
  follower 落后 live log 后通过 log replay catch-up 恢复，以及 follower 落后到 retained snapshot boundary 后通过 snapshot handoff catch-up 恢复，均已由 `RaftSnapshotCatchupTest` / `RaftIntegrationTest` 受管 CTest 覆盖。
- leader switch consistency:
  leader 切换后 committed state 保持不变、新 leader 继续推进 committed log、lagging follower 与新 proposal 混合情况下状态仍能收敛，均已由 `RaftLeaderSwitchOrderingTest` 覆盖。
- commit/apply ordering:
  `commit_index` 与 `last_applied` 的顺序边界，以及 leader switch / follower catch-up 交织时不出现 commit/apply 逆序，已由 `RaftLeaderSwitchOrderingTest` 和相关 US2 回归覆盖。
- state-machine replay consistency:
  snapshot load 后 tail replay、restart 后 committed log apply、state machine 最终视图与 committed/applied 状态一致，以及 duplicate apply / missed apply 风险，已由 `KvStateMachineTest` / `RaftSnapshotRecoveryTest` 覆盖。

### US2 Linux-Primary Observation Or Follow-Up Risk

- timing-sensitive clustered validation:
  虽然 US2 语义已由平台无关回归覆盖，但更长链路、时序更敏感的集群稳定性观察仍主要依赖 Linux 主验收入口，例如 `./test.sh --group snapshot-catchup`、`replicator`、`replication` 的低并发复验。
- retained-artifact diagnosis:
  若后续需要保留 `raft_data/`、`raft_snapshots/` 或构造现场进行人工诊断，当前仍以 Linux Bash-first 的 `--keep-data` 流程为主；这属于诊断便利性差异，不影响已记录的 US2 平台无关逻辑证据。
- cross-platform runtime follow-up:
  Windows/macOS 目前仍以 `ctest --test-dir build ...` 的平台无关逻辑回归为主，尚未补充与 Linux 主验收等价的长链路运行时观察；这仍属于 `W6/W8` 的后续范围，而不是 US2 未完成项。

## Current Risk Register

| ID | Risk | Current classification | Evidence | Follow-up task area |
|----|------|------------------------|----------|---------------------|
| R1 | Snapshot recovery timing assumption causes false negative acceptance failures | Complete but consistency-risky | `003 progress.md` blocked item 1 | T003, T005 |
| R2 | Split-brain timing assumption causes false negative acceptance failures | Complete but consistency-risky | `003 progress.md` blocked item 2 | T004, T005 |
| R3 | Exact sync/rename/prune failure boundaries are not covered by deterministic tests | Half-finished / missing industrialization ability | `003 progress.md` blocked T613-T616 | T006-T011 |
| R4 | Restart trusted-state combinations for term/vote/log/snapshot metadata/applied state regress | Accepted for US1 and protected by managed restart matrix tests | T012/T013 targeted matrices plus T014 recovery fixes | Regression only; keep evidence in T015 |
| R5 | Catch-up, leader switch, and replay consistency need stronger regression evidence | Accepted for US2 and protected by managed consistency regressions | T016/T017/T018 targeted regressions passed; T019/T020/T021 were no-op because no production defect evidence was proven | Regression only; documentation follow-up stays in T022 |
| R6 | Current validation flow is useful on Linux but not yet consolidated cross-platform | Cross-platform risk | `test.sh`, `CMakePresets.json`, `plan.md` W6 | T023-T029 |
| R7 | Windows/macOS runtime semantics for durability-related paths are still not verified | Cross-platform risk / deferred | `003 progress.md` notes, `spec.md` platform scope | T027, T031, T032 |

## Completion Classification Snapshot

### Judged Completed

- Stable Raft core behaviors already listed as protected baseline
- Segment log durability hardening from `003`
- `meta.bin` durability hardening from `003`
- Snapshot staged atomic publish hardening from `003`
- Restart recovery diagnostic strengthening from `003`
- Constructed crash-artifact recovery coverage already added in `003`
- Former manual two-phase recovery scenarios from `tests/persistence_more_test.cpp`
  are now covered by managed CTest via
  `RaftSnapshotDiagnosisTest.RestartedSingleNodeLoadsSnapshotAndTailLogsWithoutPeers`
  and
  `RaftSnapshotDiagnosisTest.CompactedClusterReplicatesNewLogAfterRestartedLeaderStepsDown`
- Restart trusted-state and log-boundary matrix coverage from T012/T014 via
  `PersistenceTest` and `RaftSegmentStorageTest` targeted restart matrices
- Snapshot metadata and applied-state restart matrix coverage from T013 via
  `RaftSnapshotRecoveryTest` and `RaftSnapshotDiagnosisTest`
- Catch-up and snapshot handoff consistency coverage from T016 via
  `RaftSnapshotCatchupTest` and `RaftIntegrationTest`
- Leader-switch and commit/apply ordering consistency coverage from T017 via
  `RaftLeaderSwitchOrderingTest`
- State-machine replay consistency coverage from T018 via
  `KvStateMachineTest` and `RaftSnapshotRecoveryTest`
- T019/T020/T021 concluded as no-op because T016/T017/T018 did not produce
  tests-first red evidence requiring production fixes
- Current US1 restart recovery acceptance has no remaining tests-first red
  cases in the managed CTest path
- Current US2 consistency acceptance has no remaining tests-first red cases in
  the managed CTest path

### Enters Follow-Up Hardening

- Linux flaky acceptance stabilization
- Exact failure injection seams and tests
- Cross-platform validation entry consolidation
- Platform support and failure-localization documentation
- Windows/macOS runtime-validation and CI follow-up definition

## Notes

- No root-level `phase-reports/` artifact is currently available for this scope.
  Carry-forward completion status therefore uses
  `specs/003-persistence-reliability/progress.md` as the authoritative phase
  baseline.
- `tests/persistence_more_test.cpp` is retained as a manual-only /
  diagnostic-only helper for two-phase rerun demos, marker files, and exported
  text snapshots/manifests. It is not part of CTest because it depends on
  re-running the same executable across phases and human inspection of retained
  artifacts.
- This document does not authorize any production-code rewrite. It only freezes
  scope and validation expectations for the remaining industrialization work.
