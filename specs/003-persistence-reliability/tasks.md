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
