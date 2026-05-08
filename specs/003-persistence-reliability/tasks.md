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
