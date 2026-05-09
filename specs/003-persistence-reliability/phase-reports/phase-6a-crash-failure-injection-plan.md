# Phase 6A: Crash / Failure Injection Tests Plan

## 1. 读取了哪些文件

- 根 `AGENTS.md`
- `modules/raft/storage/AGENTS.md`
- `modules/raft/node/AGENTS.md`
- `modules/raft/state_machine/AGENTS.md`
- `docs/PERSISTENCE_DURABILITY_CONTRACT.md`
- `specs/003-persistence-reliability/spec.md`
- `specs/003-persistence-reliability/plan.md`
- `specs/003-persistence-reliability/tasks.md`
- `specs/003-persistence-reliability/progress.md`
- `specs/003-persistence-reliability/decisions.md`
- `modules/raft/storage/raft_storage.h`
- `modules/raft/storage/raft_storage.cpp`
- `modules/raft/storage/snapshot_storage.h`
- `modules/raft/storage/snapshot_storage.cpp`
- `modules/raft/node/raft_node.cpp`
- `modules/raft/state_machine/state_machine.h`
- `modules/raft/state_machine/state_machine.cpp`
- `tests/persistence_test.cpp`
- `tests/persistence_more_test.cpp`
- `tests/test_raft_segment_storage.cpp`
- `tests/test_snapshot_storage_reliability.cpp`
- `tests/test_raft_snapshot_restart.cpp`
- `tests/test_raft_snapshot_diagnosis.cpp`
- `tests/snapshot_test.cpp`

未读取 `NOTREAD.md`、`README.md`、`deploy/` 或 `.gitignore` 覆盖目录内容。

## 2. 当前测试覆盖分析

当前测试已经覆盖以下恢复和可信状态场景：

- `tests/test_raft_segment_storage.cpp` 覆盖 segment 文件生成、compaction 后 obsolete segment 删除、tail corruption 截断、残留 `log.tmp/` / `log.bak/` 清理结果、`meta.bin` 缺失、损坏、unsupported version、meta/log boundary invariant 失败，以及 earlier segment tail corruption 后清理后续 segment。
- `tests/persistence_test.cpp` 覆盖 full cluster restart、follower restart catch-up、hard state cold restart，以及 persisted `commit_index` / `last_applied` 越界后 clamp 到 last log index。
- `tests/test_snapshot_storage_reliability.cpp` 覆盖 snapshot final layout、staging 目录忽略、缺失 meta、缺失 data、checksum mismatch、最新 invalid 回退旧 valid、同 index 同 term 幂等、prune 保留最新 snapshot。
- `tests/test_raft_snapshot_restart.cpp` 覆盖 follower 安装 snapshot 后重启、leader compacted snapshot 后重启、full cluster snapshot restart、snapshot + post-snapshot tail log recovery。
- `tests/test_raft_snapshot_diagnosis.cpp` 覆盖单节点从本地 snapshot + tail log 恢复，以及 compacted leader restart 后继续复制新 log。
- `tests/snapshot_test.cpp` 覆盖集群生成 snapshot 后重启并继续写入。

当前测试主要属于“写出完整或手工破坏后的磁盘状态，再执行 load / restart recovery”。它们能验证 trusted-state 判定，但不能精确模拟持久化操作中间失败。

## 3. Crash Window 矩阵

| 对象 | 操作 | Crash / failure window | 当前覆盖 | Phase 6B 方式 |
| --- | --- | --- | --- | --- |
| `meta.bin` | 写 `meta.bin.tmp` | 写入中途 partial write / short write | 未覆盖 | failure injection |
| `meta.bin` | `SyncFile(meta.bin.tmp)` | file fsync 失败 | 未覆盖 | failure injection |
| `meta.bin` | `ReplaceFile(meta.bin.tmp -> meta.bin)` | rename / replace 失败或 crash | 部分通过最终文件存在性间接覆盖 | failure injection + 文件构造 |
| `meta.bin` | replace 后 `data_dir` sync | directory fsync 失败 | 未覆盖 | failure injection |
| segment log | `WriteSegments(log.tmp/)` | segment write partial / checksum mismatch | tail corruption 已覆盖，写入失败未覆盖 | 文件构造 + failure injection |
| segment log | segment file fsync | file fsync 失败 | 未覆盖 | failure injection |
| segment log | `ReplaceDirectory(log.tmp -> log)` | old `log/` rename to `log.bak` 后 crash | 残留目录最终清理结果有覆盖，精确窗口未覆盖 | 文件构造 + failure injection |
| segment log | `ReplaceDirectory(log.tmp -> log)` | new `log/` publish 后、`log.bak` 清理前 crash | 部分通过 `log.bak` 残留清理结果可构造 | 文件构造 |
| segment log | parent directory sync | directory fsync 失败 | 未覆盖 | failure injection |
| segment log | recovery tail truncate | truncate 成功但 sync 失败 | truncate 行为覆盖，sync failure 未覆盖 | failure injection |
| snapshot | staging `data.bin` copy | partial copy / write failure | 缺失 / checksum mismatch 覆盖，copy failure 未覆盖 | 文件构造 + failure injection |
| snapshot | staging `data.bin` fsync | file fsync 失败 | 未覆盖 | failure injection |
| snapshot | staging `__raft_snapshot_meta` write/fsync | partial meta / fsync failure | 缺失 / invalid meta 可构造，fsync failure 未覆盖 | 文件构造 + failure injection |
| snapshot | staging dir sync | directory fsync 失败 | 未覆盖 | failure injection |
| snapshot | staging rename to final | publish rename 失败或 crash | staging 残留忽略已覆盖，rename failure 未覆盖 | 文件构造 + failure injection |
| snapshot | final parent directory sync | directory fsync 失败 | 未覆盖 | failure injection |
| snapshot | prune | remove / sync 失败 | prune success 覆盖，failure 未覆盖 | failure injection |
| restart recovery | meta/log/snapshot 组合 | 旧 meta + 新 log、新 meta + 旧 log | 未系统覆盖 | 文件构造 |
| restart recovery | snapshot fallback | 最新 invalid 回退旧 valid | 已覆盖 | 保持并纳入矩阵 |

## 4. Failure Injection 缺口

当前代码没有测试专用失败注入点，导致以下场景无法稳定覆盖：

- `SyncFile` 失败。
- `SyncDirectory` 失败。
- `std::filesystem::rename` / replace 失败。
- `std::filesystem::remove` / `remove_all` / prune 失败。
- `ofstream` / copy 过程中的 partial write、short write、EIO、disk full。
- `ReadSegmentFile()` tail truncate 后的 durability barrier 失败。
- snapshot staged publish 中 data/meta 写入成功但对应 fsync 失败。
- `FileRaftStorage::Save()` 中 `WriteSegments -> WriteMeta -> ReplaceDirectory -> ReplaceFile` 任一发布点后进程 crash。

Phase 6B 需要一个默认关闭的 test-only failure injection helper，至少能按路径、操作类型和 checkpoint name 触发失败，并把失败以现有 error channel 暴露出来。该 helper 不应改变持久化格式、协议字段、KV 行为或生产路径默认语义。

## 5. 可通过文件/目录构造完成的测试

以下场景不需要注入点，可以通过构造磁盘状态直接测试：

- `meta.bin.tmp` 残留时，restart recovery 只信任完整 `meta.bin`。
- `log.tmp/` 残留时，restart recovery 不把其作为可信 `log/`。
- `log.bak/` 残留时，restart recovery 仍以 final `log/` 和 `meta.bin` 为准。
- 旧 `meta.bin` + 新 `log/`、新 `meta.bin` + 旧 `log/` 的边界组合。
- 缺失 segment 文件、额外 segment 文件、segment filename 不连续。
- segment tail corruption 和后续 segment 清理。
- `.snapshot_staging_*` 残留、final snapshot 缺失 `data.bin`、缺失 `__raft_snapshot_meta`、checksum mismatch。
- 全部 snapshot invalid 时启动恢复应继续基于 meta/log，而不是接受不可信 snapshot。
- 最新 snapshot invalid 时回退旧 valid snapshot。

## 6. 需要 Test-Only Failure Injection Helper 的测试

以下场景需要后续引入注入能力：

- `WriteMeta` 写入成功后 `SyncFile` 失败。
- `ReplaceFile` rename 成功但 parent directory sync 失败。
- `WriteSegments` 某个 segment file sync 失败。
- `ReplaceDirectory` 在 old log rename、new log rename、backup cleanup 或 parent directory sync 失败。
- `ReadSegmentFile` tail truncate 后 `SyncFile` 失败。
- `CopyFilePortable` data copy 中途失败。
- snapshot metadata write 成功但 `SyncFile` 失败。
- staged snapshot dir sync 失败。
- staged snapshot publish rename 失败。
- snapshot final parent directory sync 失败。
- `PruneSnapshots` remove 或 directory sync 失败。

## 7. 需要修改的文件表

Phase 6B 建议优先修改：

| 文件 | 原因 |
| --- | --- |
| `tests/test_raft_segment_storage.cpp` | 增加 meta/log 文件构造类 crash recovery 测试，以及 segment failure injection 验证 |
| `tests/test_snapshot_storage_reliability.cpp` | 增加 snapshot staging/final/prune 文件构造和 failure injection 验证 |
| `tests/persistence_test.cpp` | 增加 hard state / restart recovery 组合状态测试 |
| `tests/test_raft_snapshot_restart.cpp` | 增加 snapshot + tail log crash matrix 中需要节点重启的端到端覆盖 |
| `tests/test_raft_snapshot_diagnosis.cpp` | 验证 failure 后诊断信息和 restart recovery 解释性 |
| `modules/raft/storage/raft_storage.cpp` | 仅在引入 test-only failure injection helper 时为 meta/log sync、rename、remove、truncate 注入失败点 |
| `modules/raft/storage/snapshot_storage.cpp` | 仅在引入 test-only failure injection helper 时为 snapshot copy、sync、rename、prune 注入失败点 |
| `specs/003-persistence-reliability/tasks.md` | 跟踪 Phase 6B checklist 状态 |
| `specs/003-persistence-reliability/progress.md` | 记录 Phase 6B 执行进度 |
| `specs/003-persistence-reliability/decisions.md` | 记录 failure injection 范围、默认关闭和不改格式决策 |
| `specs/003-persistence-reliability/phase-reports/phase-6-crash-failure-injection-tests.md` | Phase 6B 阶段报告 |

如果 crash matrix 独立成文档，建议新增 `specs/003-persistence-reliability/crash-matrix.md`。是否新增测试文件应由 Phase 6B 根据现有测试文件容量决定；若新增测试 target 需要接入 CMake，必须先说明原因。

## 8. 不应该修改的文件表

Phase 6B 默认不应修改：

| 文件或模块 | 原因 |
| --- | --- |
| `proto/` | Phase 6 不修改 RPC / protobuf 契约 |
| `modules/raft/service` | crash / failure injection 测试不应触碰 service 层 |
| `modules/raft/replication` | 本阶段目标不是复制状态机 |
| `modules/raft/state_machine` | 除非明确需要测试 state machine work-file failure，否则不修改 KV snapshot 编码或 KV 行为 |
| `modules/raft/node` | 仅当 restart recovery crash-point 测试必须观察节点级错误传播时才最小触碰 |
| `modules/raft/common/config.h` | 不新增运行时配置开关来承载测试注入 |
| `CMakeLists.txt` / `tests/CMakeLists.txt` | 只有新增测试文件必须接入时才允许修改，并需先说明原因 |

## 9. 需要新增或修改的测试

Phase 6B 应补充以下测试组：

- meta crash artifact tests：`meta.bin.tmp` 残留、old meta/new log、new meta/old log、meta publish failure。
- segment crash artifact tests：`log.tmp/` 残留、`log.bak/` 残留、缺失 segment、额外 segment、tail truncate sync failure。
- snapshot crash artifact tests：staging 残留、incomplete final dir、全部 invalid snapshot、latest invalid fallback、prune failure。
- restart recovery matrix tests：每个 crash artifact 恢复后只能接受最后一个完整可信状态，不能接受部分发布对象。
- failure injection tests：fsync、directory fsync、rename / replace、remove / prune、partial write failure 必须返回错误并留下可恢复状态。
- diagnostics tests：失败路径错误信息包含 path、操作类型、checkpoint 或 skip reason。

## 10. Phase 6B 最小实现计划

1. 新增或更新 crash matrix 文档，先固定覆盖目标和预期行为。
2. 先在现有测试中补文件 / 目录构造类场景，避免过早引入测试框架。
3. 对无法构造的中间失败点，引入默认关闭的 test-only failure injection helper。
4. 将注入点限制在 storage 内部 durability 操作：file sync、directory sync、rename / replace、remove / prune、write / copy。
5. 保证注入失败通过现有 error channel 返回，不改变持久化格式或生产路径默认行为。
6. 运行 persistence、segment storage、snapshot reliability、snapshot restart、snapshot diagnosis 相关测试。

## 11. 是否修改持久化格式

否。Phase 6A 不修改任何业务代码或测试代码。Phase 6B 的测试和 failure injection 设计也不应修改 `meta.bin`、segment log、snapshot data 或 snapshot metadata 的字段、编码和目录命名。

## 12. 跨平台注意事项

- POSIX 路径的 durability failure 注入应覆盖真实 `fsync` / directory `fsync` 调用点。
- Windows 路径不允许 no-op success，注入失败也必须通过错误返回暴露。
- 如果 Windows 未在实机或 CI 验证，Phase 6B 报告必须明确标记 Windows runtime semantics 未验证。
- 文件权限、rename 和 directory handle 行为存在平台差异，Phase 6B 测试不应把 POSIX-only 权限技巧当作跨平台唯一验证方式；精确失败应优先使用 test-only injection。

## 13. 明确确认没有修改业务代码

Phase 6A 仅更新任务、进度、决策和阶段分析报告；未修改 `.h` / `.cpp` / `.proto` / CMake / 测试代码，未实现 failure injection，未修改持久化格式、Raft 协议或 KV 逻辑。
