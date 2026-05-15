# Tasks: CQUPT_Raft Industrialization Hardening

**Input**: Design documents from `/specs/004-raft-industrialization/`  
**Prerequisites**: `plan.md`, `spec.md`, `research.md`, `data-model.md`, `contracts/`, `quickstart.md`

**Tests**: 本 feature 明确要求补测试、回归测试、Linux-specific crash/failure
验证和跨平台测试入口，因此任务中包含测试任务。  
**Organization**: 任务按用户故事组织，前置 Phase 只处理仍需完善的共享内容，
不会重复拆分已稳定完成的 Raft 主功能。

## Format: `[ID] [P?] [Story] Description`

- **[P]**: 可并行（不同文件、无未完成前置依赖）
- **[Story]**: 对应用户故事
- 每个任务后都附带：目标、输入、修改范围、验收标准、是否改生产代码、
  是否只改测试、是否涉及跨平台、是否 Linux-specific、是否需要
  Windows/macOS fallback、风险来源/依据、需运行测试

## Phase 1: Setup (Shared Scope Freeze)

**Purpose**: 固定只做“剩余工业化缺口”，明确与 `003-persistence-reliability`
的边界，避免后续重复规划已完成能力。

- [x] T001 Create `specs/004-raft-industrialization/validation-matrix.md`
  Goal: 建立本 feature 的单一风险与验证矩阵，冻结“已完成/补测试/需修复/跨平台风险/新增/暂缓”分类。
  Input: `specs/004-raft-industrialization/spec.md`, `plan.md`, `quickstart.md`, `specs/003-persistence-reliability/progress.md`, `test.sh`.
  Scope: 新增 `specs/004-raft-industrialization/validation-matrix.md`，不改生产代码。
  Acceptance: 文档明确列出 carried-forward 基线、P0-P4 风险来源、Linux 主入口、平台无关入口、Linux-specific 组和 deferred 项。
  Production Code: No.
  Test Only: No.
  Cross-Platform: Yes.
  Linux-Specific: No.
  Windows/macOS Fallback: Yes, 需要明确平台无关入口与未验证项。
  Basis: `plan.md` 的 W0/W1/W6/W7/W8；`specs/003-persistence-reliability/progress.md` 的 Blocked/Next/Notes。
  Tests To Run: None; 文档任务。

- [ ] T002 Update `specs/004-raft-industrialization/quickstart.md` and `specs/004-raft-industrialization/contracts/validation-entrypoints.md`
  Goal: 让 quickstart 和入口契约直接引用 `validation-matrix.md`，并收敛到真实的 Linux 与平台无关 rerun 命令。
  Input: `quickstart.md`, `contracts/validation-entrypoints.md`, `validation-matrix.md`, `test.sh`, `CMakePresets.json`.
  Scope: 仅修改 spec artifacts，不改测试和生产代码。
  Acceptance: Quickstart 中的验证顺序、rerun 命令、`--keep-data` 使用方式、Linux-specific 标签和平台无关 fallback 与矩阵保持一致。
  Production Code: No.
  Test Only: No.
  Cross-Platform: Yes.
  Linux-Specific: No.
  Windows/macOS Fallback: Yes, 必须在文档中可见。
  Basis: `research.md` Decision 5/6/7；`plan.md` W6/W7。
  Tests To Run: None; 文档任务。

---

## Phase 2: Foundational (Blocking P0 Work)

**Purpose**: 先把当前 Linux 主验收路径的噪声与失败定位面收敛，再进入更高风险的 failure injection 和恢复补强。

**⚠️ CRITICAL**: 本阶段不完成，US1 的高风险回归与 durability 任务不应关闭。

- [x] T003 Add deterministic diagnosis for `RaftSnapshotRecoveryTest` in `tests/test_raft_snapshot_restart.cpp`
  Goal: 分析并稳定 `RaftSnapshotRecoveryTest.SavesSnapshotAndRestoresAfterRestart` 的时序假设，不放宽 snapshot/restart 断言。
  Input: `tests/test_raft_snapshot_restart.cpp`, `specs/003-persistence-reliability/progress.md`, `validation-matrix.md`.
  Scope: 只改 `tests/test_raft_snapshot_restart.cpp`；必要时可增加文件内测试辅助逻辑，但不改业务行为。
  Acceptance: 该测试可重复执行而不因纯 timing 假设随机失败；失败时能看出是 leadership churn、proposal majority 还是 snapshot restore 相关。
  Production Code: No.
  Test Only: Yes.
  Cross-Platform: Indirectly.
  Linux-Specific: Yes, 当前 blocker 来源于 Linux 主验收。
  Windows/macOS Fallback: Yes, 用 `ctest --preset debug-tests` 作为平台无关回归入口，不要求同等时序证据。
  Basis: `specs/003-persistence-reliability/progress.md` Blocked 中的 `RaftSnapshotRecoveryTest...` flaky；`plan.md` W1。
  Tests To Run: `CTEST_PARALLEL_LEVEL=1 ctest --test-dir build --output-on-failure -R '^RaftSnapshotRecoveryTest\.SavesSnapshotAndRestoresAfterRestart$'`
  Result: `Passed`, `1/1 tests passed`
  Execution Note: 实际命中的测试定义位于 `tests/snapshot_test.cpp`；本次未修改生产代码。

- [x] T004 Add deterministic diagnosis for `RaftSplitBrainTest` in `tests/test_raft_split_brain.cpp`
  Goal: 稳定 `RaftSplitBrainTest.MinorityLeaderTimesOutAndDoesNotApplyUncommittedCommand` 的 leader election timing 路径，不弱化 split-brain 断言。
  Input: `tests/test_raft_split_brain.cpp`, `specs/003-persistence-reliability/progress.md`, `validation-matrix.md`.
  Scope: 只改 `tests/test_raft_split_brain.cpp`，必要时增加更明确的等待/诊断逻辑。
  Acceptance: 测试重复执行时不再因为 leader_index 时序空洞随机失败；失败信息能区分 election 未收敛、minority partition、apply 违规三类问题。
  Production Code: No.
  Test Only: Yes.
  Cross-Platform: Indirectly.
  Linux-Specific: Yes, 当前 blocker 来源于 Linux 主验收。
  Windows/macOS Fallback: Yes, 该场景在非 Linux 先走平台无关 rerun，不承诺 crash-style 等价。
  Basis: `specs/003-persistence-reliability/progress.md` Blocked 中的 `RaftSplitBrainTest...` flaky；`plan.md` W1。
  Tests To Run: `CTEST_PARALLEL_LEVEL=1 ctest --test-dir build --output-on-failure -R '^RaftSplitBrainTest\.MinorityLeaderTimesOutAndDoesNotApplyUncommittedCommand$'`
  Result: `Passed`, `1/1 tests passed`
  Execution Note: 修改文件为 `tests/test_raft_split_brain.cpp`；本次未修改生产代码；`--repeat until-fail:3` 未执行，因此无 repeat 结果可记录。

- [x] T005 Update `test.sh` comments/output and `specs/004-raft-industrialization/validation-matrix.md`
  Goal: 给当前已知 blocker 和高风险回归组补上保留数据、低并发 rerun 和失败定位提示。
  Input: `test.sh`, `validation-matrix.md`, `quickstart.md`.
  Scope: 修改脚本注释或输出提示，以及矩阵文档；不改生产代码。
  Acceptance: 维护者能从 `test.sh` 和矩阵直接看到 `snapshot-recovery`、`diagnosis`、`snapshot-catchup`、`replicator`、`segment-cluster` 的 rerun 命令和 `--keep-data` 用法。
  Production Code: No.
  Test Only: No.
  Cross-Platform: Yes.
  Linux-Specific: Partially, 因为 `test.sh` 是 Linux 主入口。
  Windows/macOS Fallback: Yes, 同步给出 `ctest --preset debug-tests` fallback。
  Basis: `plan.md` W1/W6/W7；`contracts/validation-entrypoints.md`。
  Tests To Run: `./test.sh --group snapshot-recovery --keep-data`; `./test.sh --group diagnosis`.

**Checkpoint**: Linux 主验收 blocker 有明确复现与诊断路径，US1 可进入高风险恢复与 durability 补强。

---

## Phase 3: User Story 1 - Trust Restarted State (Priority: P1, contains P0 tasks) 🎯 MVP

**Goal**: 让 committed term/vote/log/snapshot/applied state 在 crash-like 工件和 restart 下具备更可信的恢复证据。  
**Independent Test**: `CTEST_PARALLEL_LEVEL=1 ./test.sh --group persistence && ./test.sh --group snapshot-recovery && ./test.sh --group diagnosis`

### Tests for User Story 1

- [x] T006 [P] [US1] Add meta/log durability failure cases in `tests/persistence_test.cpp` and `tests/test_raft_segment_storage.cpp`
  Goal: 先写失败测试，覆盖 `meta.bin` / segment log 的 file sync、directory sync、replace/partial-write 边界。
  Input: `tests/persistence_test.cpp`, `tests/test_raft_segment_storage.cpp`, `contracts/failure-injection-boundaries.md`, `specs/003-persistence-reliability/progress.md`.
  Scope: 只改测试文件，先表达预期 trusted-state 结果和诊断要求。
  Acceptance: 新测试在 failure injection seam 未实现前可明确失败或标记待实现，且断言的是恢复边界，不是内部实现细节。
  Production Code: No.
  Test Only: Yes.
  Cross-Platform: Partially.
  Linux-Specific: Yes, 精确 durability 注入以 Linux 为主。
  Windows/macOS Fallback: Yes, 非 Linux 只保留平台无关恢复测试与文档说明。
  Basis: `specs/003-persistence-reliability/progress.md` Blocked T613-T616；`plan.md` W2/W3。
  Tests To Run: `./test.sh --group persistence`; `ctest --test-dir build --output-on-failure -R '^(PersistenceTest|RaftSegmentStorageTest)\.'`.

- [x] T007 [P] [US1] Add snapshot publish/prune failure cases in `tests/test_snapshot_storage_reliability.cpp` and `tests/test_raft_snapshot_restart.cpp`
  Goal: 先写 failure injection 测试，覆盖 snapshot publish、directory sync、prune/remove 与 restart 选择规则。
  Input: `tests/test_snapshot_storage_reliability.cpp`, `tests/test_raft_snapshot_restart.cpp`, `contracts/failure-injection-boundaries.md`, `validation-matrix.md`.
  Scope: 只改测试文件。
  Acceptance: 测试能表达 staged snapshot publish、invalid snapshot fallback、prune 失败后的 trusted-state 预期。
  Production Code: No.
  Test Only: Yes.
  Cross-Platform: Partially.
  Linux-Specific: Yes.
  Windows/macOS Fallback: Yes, 文档说明非 Linux 先不声称等价 failure injection 证据。
  Basis: `specs/003-persistence-reliability/progress.md` Blocked T613-T616；`plan.md` W2/W3。
  Tests To Run: `./test.sh --group snapshot-storage`; `./test.sh --group snapshot-recovery`.

- [x] T008 [P] [US1] Migrate useful manual recovery scenarios from `tests/persistence_more_test.cpp` into managed GTest files
  Goal: 把对工业化有价值的两阶段恢复场景纳入受管回归，而不是停留在手工诊断程序里。
  Input: `tests/persistence_more_test.cpp`, `tests/persistence_test.cpp`, `tests/test_raft_snapshot_diagnosis.cpp`, `tests/CMakeLists.txt`.
  Scope: 优先把有用场景迁入现有 GTest；若保留手工程序，则在 `tests/README.md` 或 spec docs 中明确其非回归角色。
  Acceptance: 至少一个当前只存在于 `persistence_more_test.cpp` 的恢复场景进入受管回归；剩余未迁移部分有文档说明。
  Production Code: No.
  Test Only: Yes.
  Cross-Platform: Yes.
  Linux-Specific: No.
  Windows/macOS Fallback: Not Required.
  Basis: 现状分析发现 `persistence_more_test.cpp` 未纳入 CTest；`plan.md` W3/W7。
  Tests To Run: `./test.sh --group persistence`; `ctest --test-dir build --output-on-failure -R '^PersistenceTest\.'`.

### Implementation for User Story 1

- [x] T009 [US1] Introduce an opt-in internal failure injection seam in `modules/raft/storage/raft_storage.cpp`
  Goal: 为 meta/log 的 exact failure injection 提供最小内部注入点，默认关闭且不改变成功路径语义。
  Input: `modules/raft/storage/raft_storage.cpp`, `contracts/failure-injection-boundaries.md`, `research.md`, T006.
  Scope: 仅改 `modules/raft/storage/raft_storage.cpp`；必要时新增 storage 内部 helper，但不得暴露公共 API。
  Acceptance: 注入关闭时现有测试表现不变；注入开启时能命中 fsync、directory sync、replace、partial-write 等边界。
  Production Code: Yes.
  Test Only: No.
  Cross-Platform: Yes.
  Linux-Specific: Partially, 主要证据在 Linux。
  Windows/macOS Fallback: Yes, 需要明示 weaker guarantee 或 deferred runtime validation。
  Basis: `research.md` Decision 3；`plan.md` W2；`specs/003-persistence-reliability/progress.md` Blocked T613-T616。
  Tests To Run: T006 的新增用例；`./test.sh --group persistence`.

- [x] T010 [US1] Introduce the same opt-in seam for snapshot publish/prune in `modules/raft/storage/snapshot_storage.cpp`
  Goal: 覆盖 snapshot data/meta publish、directory sync 和 prune/remove 失败点。
  Input: `modules/raft/storage/snapshot_storage.cpp`, `contracts/failure-injection-boundaries.md`, T007.
  Scope: 仅改 `modules/raft/storage/snapshot_storage.cpp`；必要时复用 T009 的内部 helper。
  Acceptance: snapshot publish/prune 的 failure injection 可稳定触发，默认关闭时不改变现有语义与格式。
  Production Code: Yes.
  Test Only: No.
  Cross-Platform: Yes.
  Linux-Specific: Partially.
  Windows/macOS Fallback: Yes.
  Basis: `research.md` Decision 3/6；`plan.md` W2；`specs/003-persistence-reliability/progress.md` Blocked T613-T616。
  Tests To Run: T007 的新增用例；`./test.sh --group snapshot-storage`; `./test.sh --group snapshot-recovery`.

- [x] T011 [US1] Add injected-failure diagnostics in `modules/raft/storage/raft_storage.cpp` and `modules/raft/storage/snapshot_storage.cpp`
  Goal: 让 durability failure 的日志/诊断能指明操作、路径、平台范围和 trusted-state 预期。
  Input: `raft_storage.cpp`, `snapshot_storage.cpp`, `validation-matrix.md`, T009, T010.
  Scope: 仅改 storage 相关 `.cpp`。
  Acceptance: 失败输出能区分 file sync、directory sync、replace/rename、prune/remove、partial write；文档中可映射到恢复预期。
  Production Code: Yes.
  Test Only: No.
  Cross-Platform: Yes.
  Linux-Specific: Partially.
  Windows/macOS Fallback: Yes.
  Basis: `research.md` Decision 7；`plan.md` W2/W7。
  Tests To Run: `./test.sh --group persistence --keep-data`; `./test.sh --group diagnosis`.

- [x] T012 [US1] Add hard-state and log-boundary restart matrix tests in `tests/persistence_test.cpp` and `tests/test_raft_segment_storage.cpp`
  Goal: 系统化补齐 `current_term`、`voted_for`、`commit_index`、`last_applied` 与 log boundary 的重启恢复组合。
  Input: `tests/persistence_test.cpp`, `tests/test_raft_segment_storage.cpp`, `data-model.md`, `validation-matrix.md`.
  Scope: 只改测试文件。
  Acceptance: 至少覆盖 old-meta/new-log、new-meta/old-log、boundary mismatch、tail truncate 后 trusted-state 选择等组合。
  Production Code: No.
  Test Only: Yes.
  Cross-Platform: Yes.
  Linux-Specific: No.
  Windows/macOS Fallback: Not Required.
  Basis: `plan.md` W3；`spec.md` FR-003/FR-010；`specs/003-persistence-reliability/progress.md` Notes about trusted-state rules。
  Tests To Run: `./test.sh --group persistence`; `./test.sh --group segment-basic`.

- [x] T013 [P] [US1] Add snapshot metadata and applied-state restart matrix tests in `tests/test_raft_snapshot_restart.cpp` and `tests/test_raft_snapshot_diagnosis.cpp`
  Goal: 补齐 invalid snapshot、all-invalid fallback、metadata mismatch、applied replay 的恢复覆盖。
  Input: `tests/test_raft_snapshot_restart.cpp`, `tests/test_raft_snapshot_diagnosis.cpp`, `data-model.md`, `validation-matrix.md`.
  Scope: 只改测试文件。
  Acceptance: 新测试能验证非法 snapshot 不会被误接受，恢复后 apply/replay 与 trusted snapshot 选择一致。
  Production Code: No.
  Test Only: Yes.
  Cross-Platform: Yes.
  Linux-Specific: No.
  Windows/macOS Fallback: Not Required.
  Basis: `plan.md` W3/W5；`spec.md` FR-003；`specs/003-persistence-reliability/progress.md` Phase 5/6 notes。
  Tests To Run: `./test.sh --group snapshot-recovery`; `./test.sh --group diagnosis`.

- [x] T014 [US1] Apply minimal recovery fixes in `modules/raft/storage/raft_storage.cpp`, `modules/raft/storage/snapshot_storage.cpp`, or `modules/raft/node/raft_node.cpp` if T012-T013 expose real defects
  Goal: 仅在测试证明存在真实 trusted-state/restart 缺陷时做最小修复，不改格式、不改协议、不扩展公共 API。
  Input: T012/T013 failing evidence, affected `.cpp` files, `plan.md`, constitution.
  Scope: 限定在相关 `.cpp`；若触及 `raft_node.cpp`，必须同时检查相关 storage/state_machine tests。
  Acceptance: 对应失败测试转绿；未引入格式变更、协议变更或额外回归。
  Production Code: Yes.
  Test Only: No.
  Cross-Platform: Yes.
  Linux-Specific: No.
  Windows/macOS Fallback: Yes, 若修复依赖平台差异要写入矩阵。
  Basis: `plan.md` W3；constitution II/III；`spec.md` FR-010。
  Tests To Run: T012/T013 相关测试；`./test.sh --group persistence`; `./test.sh --group snapshot-recovery`; `./test.sh --group diagnosis`.


- [x] T015 [US1] Update `specs/004-raft-industrialization/validation-matrix.md` with accepted US1 evidence
  Goal: 把 Linux-specific durability 注入证据、平台无关恢复证据和 deferred Windows/macOS 范围正式记录下来。
  Input: T006-T014 results, `validation-matrix.md`, `quickstart.md`.
  Scope: 文档更新。
  Acceptance: 矩阵能回答“哪些恢复能力已证实”“哪些只在 Linux 验证”“哪些仍需 Windows/macOS follow-up”。
  Production Code: No.
  Test Only: No.
  Cross-Platform: Yes.
  Linux-Specific: Partially.
  Windows/macOS Fallback: Yes.
  Basis: `plan.md` W2/W3/W7/W8；`contracts/failure-injection-boundaries.md`。
  Tests To Run: None; 文档任务。

**Checkpoint**: US1 完成后，重启恢复和 crash-like trusted-state 边界应可独立验证，是本 feature 的 MVP。

---

## Phase 4: User Story 2 - Preserve Cluster Consistency Under Lag And Leadership Change (Priority: P2)

**Goal**: 补强 follower lag/restart、snapshot handoff、post-snapshot replay、leader 切换和 apply/restart 一致性证据。  
**Independent Test**: `CTEST_PARALLEL_LEVEL=1 ./test.sh --group snapshot-catchup && ./test.sh --group replicator && ./test.sh --group replication && ./test.sh --group snapshot-restart`

### Tests for User Story 2

- [x] T016 [P] [US2] Add lagging-follower and compaction catch-up regressions in `tests/test_raft_snapshot_catchup.cpp` and `tests/raft_integration_test.cpp`
  Goal: 覆盖 follower 落后 live log 和 retained snapshot boundary 两类 catch-up 场景。
  Input: `tests/test_raft_snapshot_catchup.cpp`, `tests/raft_integration_test.cpp`, `plan.md`, `validation-matrix.md`.
  Scope: 只改测试文件。
  Acceptance: 新测试可区分 log replay catch-up 与 snapshot handoff catch-up，并验证 committed ordering 不被破坏。
  Production Code: No.
  Test Only: Yes.
  Cross-Platform: Yes.
  Linux-Specific: No.
  Windows/macOS Fallback: Not Required.
  Basis: `plan.md` W4；`spec.md` FR-004；现状分析中 follower catch-up 仍属需补强区域。
  Tests To Run: `./test.sh --group snapshot-catchup`; `./test.sh --group integration`.

- [x] T017 [P] [US2] Add leader-switch and commit/apply ordering regressions in `tests/test_raft_replicator_behavior.cpp`, `tests/test_raft_log_replication.cpp`, and `tests/test_raft_commit_apply.cpp`
  Goal: 覆盖 leader 切换后 committed state 保持不变、新日志继续一致推进、commit/apply 不逆序。
  Input: `tests/test_raft_replicator_behavior.cpp`, `tests/test_raft_log_replication.cpp`, `tests/test_raft_commit_apply.cpp`, `spec.md`.
  Scope: 只改测试文件。
  AcceptanceX: 测试能在 leader 变化、follower 追赶和新 proposal 混合情况下验证一致性，而不是只验证单一 steady-state 路径。
  Production Code: No.
  Test Only: Yes.
  Cross-Platform: Yes.
  Linux-Specific: No.
  Windows/macOS Fallback: Not Required.
  Basis: `plan.md` W4；`spec.md` FR-004；US2 acceptance scenarios。
  Tests To Run: `./test.sh --group replication`; `./test.sh --group replicator`.

- [x] T018 [P] [US2] Add state-machine replay consistency regressions in `tests/test_state_machine.cpp` and `tests/test_raft_snapshot_restart.cpp`
  Goal: 覆盖 snapshot load 后 replay、restart 后 apply、state machine 视图一致性边界。
  Input: `tests/test_state_machine.cpp`, `tests/test_raft_snapshot_restart.cpp`, `plan.md`, `data-model.md`.
  Scope: 只改测试文件。
  Acceptance: 新测试能发现 duplicate apply、missed apply、snapshot load 后 tail replay 丢失等问题。
  Production Code: No.
  Test Only: Yes.
  Cross-Platform: Yes.
  Linux-Specific: No.
  Windows/macOS Fallback: Not Required.
  Basis: `plan.md` W5；`spec.md` FR-003/FR-004。
  Tests To Run: `ctest --test-dir build --output-on-failure -R '^(KvStateMachineTest|RaftSnapshotRecoveryTest)\.'`; `./test.sh --group snapshot-recovery`.

### Implementation for User Story 2

- [x] T019 [US2] Apply minimal catch-up fixes in `modules/raft/replication/replicator.cpp` if T016 reveals real defects
  Goal: 仅在新回归证明 follower catch-up 边界有缺陷时，最小修复 replicator 的回退、snapshot handoff 或 next-index 推进逻辑。
  Input: T016 failing evidence, `modules/raft/replication/replicator.cpp`, related existing tests.
  Scope: 仅改 `modules/raft/replication/replicator.cpp`。
  Acceptance: T016 相关失败用例转绿，现有 replicator 行为测试不回归。
  Production Code: Yes.
  Test Only: No.
  Cross-Platform: Yes.
  Linux-Specific: No.
  Windows/macOS Fallback: Yes, 修复结果必须通过平台无关测试入口。
  Basis: `plan.md` W4；constitution III；现状分析中的 catch-up industrialization gap。
  Tests To Run: `./test.sh --group snapshot-catchup`; `./test.sh --group replicator`.
  Execution Note: No-op。T016 的 log replay catch-up 与 snapshot handoff catch-up 回归测试已通过，未发现 `modules/raft/replication/replicator.cpp` 的真实实现缺口；本任务未修改生产代码。

- [x] T020 [US2] Apply minimal leader-switch or orchestration fixes in `modules/raft/node/raft_node.cpp` if T017 reveals real defects
  Goal: 在不改变协议语义的前提下，修复 leader 切换、commit/apply 顺序或 restart 后推进边界的真实缺陷。
  Input: T017 failing evidence, `modules/raft/node/raft_node.cpp`, related storage/replication tests.
  Scope: 限定改 `raft_node.cpp`；按 AGENTS 规则同步检查 `replication`、`storage`、`state_machine` 相关测试。
  Acceptance: T017 相关失败用例转绿，稳定选举/复制/commit 基线不被重排为新实现。
  Production Code: Yes.
  Test Only: No.
  Cross-Platform: Yes.
  Linux-Specific: No.
  Windows/macOS Fallback: Yes.
  Basis: `plan.md` W4；constitution III；`spec.md` US2 acceptance scenario 2。
  Tests To Run: `./test.sh --group replication`; `./test.sh --group election`; `./test.sh --group replicator`.
  Execution Note: No-op。T017 的 leader-switch / commit-apply ordering 回归测试已通过，未发现 `modules/raft/node/raft_node.cpp` 的真实实现缺口；本任务未修改生产代码。

- [x] T021 [US2] Apply minimal replay fixes in `modules/raft/state_machine/state_machine.cpp` or `modules/raft/node/raft_node.cpp` if T018 reveals real defects
  Goal: 修复 apply/replay 与 snapshot load 边界上的真实一致性问题，但不修改状态机格式或公共语义。
  Input: T018 failing evidence, `state_machine.cpp`, `raft_node.cpp`, existing snapshot restart tests.
  Scope: 限定在 `state_machine.cpp` 和必要的 `raft_node.cpp`。
  Acceptance: state machine restart/replay 新增用例转绿，现有 snapshot/save/load/restart 基线不回归。
  Production Code: Yes.
  Test Only: No.
  Cross-Platform: Yes.
  Linux-Specific: No.
  Windows/macOS Fallback: Yes.
  Basis: `plan.md` W5；`spec.md` FR-003；现状分析中的 apply/recovery consistency gap。
  Tests To Run: `ctest --test-dir build --output-on-failure -R '^(KvStateMachineTest|RaftSnapshotRecoveryTest|RaftIntegrationTest)\.'`; `./test.sh --group snapshot-recovery`.
  Execution Note: No-op。T018 的 state-machine replay consistency 回归测试已通过，未发现 `modules/raft/state_machine/state_machine.cpp` 或 `modules/raft/node/raft_node.cpp` 的真实实现缺口；本任务未修改生产代码。

- [x] T022 [US2] Update `specs/004-raft-industrialization/validation-matrix.md` with US2 consistency evidence
  Goal: 记录 catch-up、leader switch、apply/replay 一致性的新增回归证据与未覆盖时序风险。
  Input: T016-T021 results, `validation-matrix.md`.
  Scope: 文档更新。
  Acceptance: 矩阵中能区分“已由平台无关回归覆盖”和“仍只在 Linux 主验收上观察”的场景。
  Production Code: No.
  Test Only: No.
  Cross-Platform: Yes.
  Linux-Specific: Partially.
  Windows/macOS Fallback: Yes.
  Basis: `plan.md` W4/W5/W7；`data-model.md` ValidationScope。
  Tests To Run: None; 文档任务。

**Checkpoint**: US2 完成后，lag/restart/leader-switch 一致性应能独立验收，而不依赖新协议或大范围重构。

---

## Phase 5: User Story 3 - Run A Clear Industrialization Validation Flow (Priority: P3)

**Goal**: 建立清晰的一键验证入口、Linux-specific 隔离、跨平台 fallback 和失败定位文档。  
**Independent Test**: `cmake --preset debug-ninja-low-parallel && cmake --build --preset debug-ninja-low-parallel && CTEST_PARALLEL_LEVEL=1 ./test.sh --group all && ctest --preset debug-tests --output-on-failure`

### Implementation for User Story 3

- [x] T023 [P] [US3] Refine `test.sh` into explicit platform-neutral vs Linux-specific sections
  Goal: 让 Bash 主入口清楚标记 Linux-specific 组、平台无关组、`--keep-data` 和失败 rerun 指南。
  Input: `test.sh`, `validation-matrix.md`, `contracts/validation-entrypoints.md`.
  Scope: 只改 `test.sh` 和相关 spec 文档。
  Acceptance: 维护者从脚本头部或输出就能看出哪些组属于 Linux-specific，哪些是平台无关基础回归。
  Production Code: No.
  Test Only: No.
  Cross-Platform: Yes.
  Linux-Specific: Yes.
  Windows/macOS Fallback: Yes, 必须同步给出非 Bash fallback。
  Basis: `research.md` Decision 5/6；`plan.md` W6/W7。
  Tests To Run: `./test.sh --group all`; `./test.sh --group persistence`.

- [x] T024 [P] [US3] Extend `CMakePresets.json` and adjust top-level `CMakeLists.txt` only if needed for non-hardcoded Windows configuration
  Goal: 提供更明确的跨平台 configure/build/test 入口，减少当前仅 Linux/Ninja 友好的假设；只有在必要时才修改 `CMakeLists.txt` 去掉 Windows 本机路径耦合。
  Input: `CMakePresets.json`, top-level `CMakeLists.txt`, `contracts/validation-entrypoints.md`, `validation-matrix.md`.
  Scope: 首选修改 presets；仅在 preset 无法解决 Windows 路径耦合时，最小修改 `CMakeLists.txt` 的工具链/依赖注入方式。
  Acceptance: Linux 现有 preset 不受破坏；新增或调整后的入口不再要求硬编码本机 Windows 路径才能理解如何配置。
  Production Code: No.
  Test Only: No.
  Cross-Platform: Yes.
  Linux-Specific: No.
  Windows/macOS Fallback: Yes, 这是本任务核心内容。
  Basis: 现状分析中的 `CMakeLists.txt` Windows 硬编码风险；`plan.md` W6/W8。
  Tests To Run: `cmake --preset debug-ninja-low-parallel`; `cmake --build --preset debug-ninja-low-parallel`; `ctest --preset debug-tests --output-on-failure`.

- [ ] T025 [P] [US3] Add non-Bash fallback runner in `test.ps1`
  Goal: 提供 Windows/macOS 规划可参考的非 Bash 测试入口，至少覆盖平台无关的 CMake/CTest 流程。
  Input: `test.sh`, `CMakePresets.json`, `contracts/validation-entrypoints.md`, `validation-matrix.md`.
  Scope: 新增 `test.ps1`；必要时更新相关文档。
  Acceptance: `test.ps1` 明确走平台无关的 configure/build/test 流程，并显式排除 Linux-specific crash/failure groups。
  Production Code: No.
  Test Only: No.
  Cross-Platform: Yes.
  Linux-Specific: No.
  Windows/macOS Fallback: Yes, 这是任务目标。
  Basis: `contracts/validation-entrypoints.md` Windows/macOS wrapper contract；`plan.md` W6/W8。
  Tests To Run: 文档/脚本静态校验；Linux 上至少检查脚本文本与命令一致性，实际 Windows 运行留作 follow-up evidence。

- [ ] T026 [US3] Update `tests/CMakeLists.txt` and related docs to label Linux-specific failure-injection coverage
  Goal: 让 CTest discover 的测试和文档都能看出哪些是平台无关回归，哪些依赖 Linux-specific failure injection 或 crash-style 语义。
  Input: `tests/CMakeLists.txt`, `validation-matrix.md`, `contracts/validation-entrypoints.md`, T006-T010.
  Scope: 修改 `tests/CMakeLists.txt` 的注释/组织和 spec docs；不改业务代码。
  Acceptance: 新增/更新的 failure-injection 测试在任务文档中有清晰标签；平台无关运行说明不再隐式包含 Linux-only 场景。
  Production Code: No.
  Test Only: No.
  Cross-Platform: Yes.
  Linux-Specific: Partially.
  Windows/macOS Fallback: Yes.
  Basis: `plan.md` W6；`contracts/validation-entrypoints.md` Test Grouping Contract。
  Tests To Run: `ctest --preset debug-tests --output-on-failure`; `./test.sh --group all`.

- [ ] T027 [P] [US3] Add `specs/004-raft-industrialization/platform-support.md`
  Goal: 建立平台能力矩阵，明确 build、path、flush/sync、rename、temp dir、signal/process 测试在 Linux/Windows/macOS 的当前证据与下一步。
  Input: `validation-matrix.md`, `plan.md`, `data-model.md`, top-level `CMakeLists.txt`, `test.sh`.
  Scope: 新增 spec 文档，不改生产代码。
  Acceptance: 文档逐项说明 Linux 当前能力、Windows/macOS 当前状态、当前证据、下一步 follow-up；不虚报跨平台等价。
  Production Code: No.
  Test Only: No.
  Cross-Platform: Yes.
  Linux-Specific: No.
  Windows/macOS Fallback: Yes.
  Basis: `data-model.md` PlatformSupportExpectation；`plan.md` W7/W8；用户跨平台要求。
  Tests To Run: None; 文档任务。

- [ ] T028 [US3] Update `specs/004-raft-industrialization/quickstart.md` and `tests/README.md`
  Goal: 给维护者一份清晰的“从哪跑、失败后看哪、哪些数据可保留、哪些测试不进平台无关回归”的说明。
  Input: `quickstart.md`, `tests/README.md`, `validation-matrix.md`, `platform-support.md`.
  Scope: 仅修改文档。
  Acceptance: Linux 主入口、平台无关入口、PowerShell fallback、保留数据路径、manual-only test 程序角色都有明确说明。
  Production Code: No.
  Test Only: No.
  Cross-Platform: Yes.
  Linux-Specific: Partially.
  Windows/macOS Fallback: Yes.
  Basis: `research.md` Decision 5/7；`plan.md` W6/W7/W8。
  Tests To Run: None; 文档任务。

- [ ] T029 [US3] Run the Linux primary path and platform-neutral fallback and record interpretation rules in `specs/004-raft-industrialization/validation-matrix.md`
  Goal: 用真实执行结果确认维护者看到失败时如何判断是 Linux-specific、平台无关、还是 deferred runtime gap。
  Input: `validation-matrix.md`, `quickstart.md`, `test.sh`, `CMakePresets.json`.
  Scope: 执行现有测试并更新文档；不改生产代码。
  Acceptance: 文档包含 `./test.sh --group all`、`ctest --preset debug-tests` 的解释规则和失败转诊路径。
  Production Code: No.
  Test Only: No.
  Cross-Platform: Yes.
  Linux-Specific: Partially.
  Windows/macOS Fallback: Yes.
  Basis: `plan.md` W6/W7；`spec.md` FR-007/FR-008/FR-011。
  Tests To Run: `cmake --preset debug-ninja-low-parallel`; `cmake --build --preset debug-ninja-low-parallel`; `CTEST_PARALLEL_LEVEL=1 ./test.sh --group all`; `ctest --preset debug-tests --output-on-failure`.

**Checkpoint**: US3 完成后，维护者应能独立理解 Linux 主入口、平台无关 fallback、Windows/macOS follow-up 边界和失败定位路径。

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: 收口跨故事文档、遗留脚本/测试角色说明与 P4 follow-up，不做大重构。

- [ ] T030 [P] Document the role of `tests/persistence_more_test.cpp` and `tests/test_temp.cpp` in `tests/README.md` and `specs/004-raft-industrialization/validation-matrix.md`
  Goal: 区分受管回归测试、手工诊断程序、临时测试文件，避免后续把非回归程序误当正式验收资产。
  Input: `tests/persistence_more_test.cpp`, `tests/test_temp.cpp`, `tests/README.md`, `validation-matrix.md`.
  Scope: 文档为主；若决定删除临时文件，需另开 feature，不在本任务直接做大清理。
  Acceptance: 文档明确说明哪些文件参与 CTest，哪些只是手工/临时用途。
  Production Code: No.
  Test Only: No.
  Cross-Platform: Yes.
  Linux-Specific: No.
  Windows/macOS Fallback: Not Required.
  Basis: 当前目录结构分析；`plan.md` W7。
  Tests To Run: None; 文档任务。

- [ ] T031 [P] Cross-check Linux-specific tasks against `specs/004-raft-industrialization/platform-support.md`
  Goal: 确保所有 Linux-specific failure injection、crash-style、bash-only 任务都有 Windows/macOS fallback 或 deferred note。
  Input: `tasks.md`, `validation-matrix.md`, `platform-support.md`, `contracts/`.
  Scope: 文档交叉检查。
  Acceptance: 没有任何 Linux-specific 任务缺少 fallback/未验证声明；`spec.md` SC-004 可以被文档证明。
  Production Code: No.
  Test Only: No.
  Cross-Platform: Yes.
  Linux-Specific: No.
  Windows/macOS Fallback: Yes.
  Basis: constitution IV；`plan.md` W8；`spec.md` SC-004。
  Tests To Run: None; 文档任务。

- [ ] T032 Run final acceptance sweep from `specs/004-raft-industrialization/quickstart.md`
  Goal: 用最终任务成果执行一次统一收口，确认 P0-P3 范围都有对应证据，P4 只留下明确 follow-up。
  Input: `quickstart.md`, `validation-matrix.md`, `platform-support.md`, all prior task outputs.
  Scope: 执行测试并更新文档结论，不改业务逻辑。
  Acceptance: 最终文档能说明哪些任务完成、哪些任务只在 Linux 验证、哪些 Windows/macOS 仍待 follow-up；所有高风险改动都有对应回归。
  Production Code: No.
  Test Only: No.
  Cross-Platform: Yes.
  Linux-Specific: Partially.
  Windows/macOS Fallback: Yes.
  Basis: `plan.md` Acceptance By Priority；`quickstart.md` implementation order。
  Tests To Run: `cmake --preset debug-ninja-low-parallel`; `cmake --build --preset debug-ninja-low-parallel`; `CTEST_PARALLEL_LEVEL=1 ./test.sh --group all --keep-data`; `ctest --preset debug-tests --output-on-failure`.

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1: Setup**: 无依赖，立即开始。
- **Phase 2: Foundational**: 依赖 Phase 1，阻塞 US1 的高风险实现。
- **Phase 3: US1**: 依赖 Phase 2，是 MVP，先完成 P0 恢复/持久化可信度。
- **Phase 4: US2**: 依赖 US1 的 trusted-state 基线，避免把恢复问题与复制问题混在一起。
- **Phase 5: US3**: 依赖 US1/US2 的实际产出，才能形成真实入口和文档。
- **Phase 6: Polish**: 所有目标故事完成后收口。

### User Story Dependencies

- **US1**: 依赖 Foundational；不依赖 US2/US3。
- **US2**: 依赖 US1 的 trusted-state 与恢复基线。
- **US3**: 依赖 US1/US2 的真实测试与修复结果，但不应反向阻塞 P0 修复。

### Within Each User Story

- 先写或补失败测试，再定是否需要生产代码修复。
- `storage` failure injection 与 trusted-state 测试在 `node`/`replication` 修复前先落地。
- 文档和平台说明在对应验证证据形成后再更新。

### Priority Order

- **P0**: T001-T015
- **P1**: T016-T022
- **P2**: T023-T026
- **P3**: T027-T030
- **P4**: T031-T032

### Parallel Opportunities

- T001 和 T002 可并行。
- T003 和 T004 可并行；T005 在两者结果出来后更新脚本/文档。
- T006、T007、T008 可并行。
- T012 和 T013 可并行。
- T016、T017、T018 可并行。
- T023、T024、T025、T027 可并行，因为分别落在脚本、presets/CMake、PowerShell、平台文档。

---

## Parallel Example: User Story 1

```bash
# Tests-first tasks for US1 can run together:
Task: "Add meta/log durability failure cases in tests/persistence_test.cpp and tests/test_raft_segment_storage.cpp"
Task: "Add snapshot publish/prune failure cases in tests/test_snapshot_storage_reliability.cpp and tests/test_raft_snapshot_restart.cpp"
Task: "Migrate useful manual recovery scenarios from tests/persistence_more_test.cpp into managed GTest files"

# After the test shape is clear, these can run in parallel:
Task: "Introduce an opt-in internal failure injection seam in modules/raft/storage/raft_storage.cpp"
Task: "Introduce the same opt-in seam for snapshot publish/prune in modules/raft/storage/snapshot_storage.cpp"
```

---

## Parallel Example: User Story 2

```bash
# Regression additions for US2 can run together:
Task: "Add lagging-follower and compaction catch-up regressions in tests/test_raft_snapshot_catchup.cpp and tests/raft_integration_test.cpp"
Task: "Add leader-switch and commit/apply ordering regressions in tests/test_raft_replicator_behavior.cpp, tests/test_raft_log_replication.cpp, and tests/test_raft_commit_apply.cpp"
Task: "Add state-machine replay consistency regressions in tests/test_state_machine.cpp and tests/test_raft_snapshot_restart.cpp"
```

---

## Implementation Strategy

### MVP First (US1)

1. 完成 Phase 1 和 Phase 2。
2. 完成 US1 的 tests-first + minimal storage/restart hardening。
3. 单独执行 US1 的 persistence/snapshot-recovery/diagnosis 验收。
4. 若 US1 证据成立，再进入 US2。

### Incremental Delivery

1. Setup + Foundational：冻结范围并稳定当前 Linux blocker。
2. US1：建立 restart/trusted-state/durability 可信度。
3. US2：补强 cluster consistency 与 state-machine replay 证据。
4. US3：收敛一键入口、平台 fallback 与失败定位文档。
5. Polish：完成 P4 follow-up 标注与最终收口。

## Notes

- 所有任务都只针对“仍需完善”的工业化缺口，没有把稳定完成的 Raft 主路径重新拆成实现任务。
- 任何生产代码任务都限定在最小 `.cpp` 修改范围，并以新增失败测试为前置证据。
- Linux-specific 任务都必须在文档中带 Windows/macOS fallback 或 deferred note。
