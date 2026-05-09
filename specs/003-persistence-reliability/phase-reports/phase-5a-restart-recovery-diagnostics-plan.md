# Phase 5A: Restart Recovery Validation And Diagnostics Plan

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
- `modules/raft/node/raft_node.h`
- `modules/raft/node/raft_node.cpp`
- `modules/raft/state_machine/state_machine.h`
- `modules/raft/state_machine/state_machine.cpp`
- `tests/persistence_test.cpp`
- `tests/test_raft_segment_storage.cpp`
- `tests/test_snapshot_storage_reliability.cpp`
- `tests/test_raft_snapshot_restart.cpp`
- `tests/test_raft_snapshot_diagnosis.cpp`
- `tests/snapshot_test.cpp`

未读取 `NOTREAD.md`、`README.md`、`deploy/` 或 `.gitignore` 覆盖目录内容。

## 2. 当前 restart recovery 路径分析

`RaftNode` 构造函数是 restart recovery 的编排入口。当前恢复顺序是先通过 `storage_->Load()` 恢复 `meta.bin + log/`，再在启用 snapshot startup load 时调用 `LoadLatestSnapshotOnStartup()`，最后在 `commit_index_ > last_applied_` 时调用 `ApplyCommittedEntries()` replay committed tail log。

`FileRaftStorage::Load()` 优先加载 `meta.bin` 和 segmented log；如果 `meta.bin` 不存在但 legacy `raft_state.bin` 存在，则走 legacy 恢复；如果两者都不存在，则以 empty state 启动。segmented 路径中，`LoadSegmented()` 先 `ReadMeta()` 读取 hard state 与 log 边界，再 `LoadSegments()` 按 meta 期望的 first/last/count 加载 segment records。

segment log 恢复由 `LoadSegments()` 和 `ReadSegmentFile()` 负责。`LoadSegments()` 枚举 `segment_*.log`，忽略非 segment 文件，按起始 index 排序并校验 count、boundary、continuity。`ReadSegmentFile()` 对每条记录校验 header、payload size 和 checksum；遇到 partial header、partial payload、invalid header、payload 过大或 checksum mismatch 时，会 truncate 到 last good offset，并在后续 segment 上执行清理。

snapshot startup 恢复由 `FileSnapshotStorage::ListSnapshots()` 与 `RaftNode::LoadLatestSnapshotOnStartup()` 组合完成。`ListSnapshots()` 只返回有效 snapshot，目录 snapshot 需要满足目录名、metadata、data 文件存在和 checksum 匹配；staging/temp 目录不会作为 trusted snapshot 返回。node 层按返回顺序从最新有效 snapshot 开始尝试 `state_machine_->LoadSnapshot()`，成功后 compact log prefix、提升 `commit_index_` 与 `last_applied_`，再持久化当前状态。

state machine snapshot 加载由 `KvStateMachine::LoadSnapshot()` 完成。它校验 state machine snapshot 文件存在、header magic/version 和 KV entries，只有完整读完后才替换内存 KV 状态。

## 3. 当前 trusted-state 判定规则

- `meta.bin` 必须能完整解析 magic/version 与 hard state 字段。
- segment log 必须满足 `meta.bin` 声明的 `first_log_index`、`last_log_index` 和 `log_count`。
- segment records 必须连续；坏 tail 可被截断，但最终 records count 和边界仍需满足 meta 期望，否则恢复失败。
- snapshot 必须通过 catalog 层 validation：目录名 index 与 metadata index 一致、metadata 可解析、`data.bin` 存在、checksum 匹配。
- snapshot 选择规则是“最新有效优先”；无效 snapshot 在 catalog 层被跳过，state machine load 失败的 snapshot 在 node 层被跳过并继续尝试更旧 snapshot。
- snapshot 加载成功后，状态机先恢复到 snapshot index，然后 replay committed tail log。
- recovery 后 `commit_index_` 来自 persisted commit 与 persisted last_applied 的 max，并会被 last log index clamp；`last_applied_` 启动时不直接信任 persisted value，而是从 snapshot load 和 committed replay 推进。

## 4. 当前 recovery 诊断缺口

- `ReadMeta()` 的字段读取错误缺少字段名和 path 上下文，meta boundary invalid 的错误不够细。
- `LoadSegments()` 的 count、boundary、continuity 错误缺少 meta expected values、实际 segment 文件列表和实际 accepted records 范围。
- `ReadSegmentFile()` 成功处理坏 tail 后缺少明确诊断，不易回答为什么 truncate、truncate 到哪里、哪些后续 segment 被删除。
- `ListSnapshots()` 静默过滤 invalid snapshot，node startup 只能看到 filtered valid list，无法解释某个 snapshot 为什么被跳过。
- `LoadLatestSnapshotOnStartup()` 对候选数量、最终选择、全部无效、无 snapshot 可用等情况缺少 summary 诊断。
- state machine snapshot load error 包含文件级原因，但缺少对应 snapshot index/term/catalog metadata 上下文。
- startup replay 失败时虽然会抛出错误，但缺少 `commit_index`、`last_applied`、`last_snapshot_index`、last log index 和 replay range 汇总。
- recovery 成功后缺少统一状态摘要，不能直接从日志判断 commit/apply/snapshot/log 边界是否一致。

## 5. 当前 recovery 测试缺口

- 缺少 meta boundary invariant 测试，例如 `log_count` 与 `first_log_index` / `last_log_index` 不一致。
- 缺少 unsupported meta version、meta 字段截断、commit/applied 越界 clamp 的诊断测试。
- 缺少 segment tail truncate 的诊断测试，尤其是 truncate reason、offset 与后续 segment cleanup。
- 缺少 snapshot skip reason 测试，当前只验证 invalid snapshot 不被信任或能回退到旧 snapshot。
- 缺少全部 snapshot invalid 时 startup 行为与诊断测试。
- 缺少多 snapshot 场景下“为什么选择这个 snapshot”的候选 summary 测试。
- 缺少 recovery 后状态摘要测试，覆盖 `commit_index`、`last_applied`、`last_snapshot_index`、`last_snapshot_term` 和 tail log replay range。
- 已观察到 `RaftSnapshotRecoveryTest.SavesSnapshotAndRestoresAfterRestart` 存在 leader churn / propose failure 稳定性问题，应在 Phase 5B 作为 recovery 测试诊断问题处理，不能删除或跳过。

## 6. 需要修改的文件表

| 文件 | Phase 5B 预期用途 |
| --- | --- |
| `modules/raft/storage/raft_storage.cpp` | 增加 meta/log load 边界校验诊断、segment tail truncate 诊断和错误上下文 |
| `modules/raft/storage/raft_storage.h` | 仅当需要暴露诊断结构或测试入口时才修改；默认不改 |
| `modules/raft/storage/snapshot_storage.cpp` | 暴露或记录 snapshot validation skip reason，保留 trusted snapshot 选择语义 |
| `modules/raft/storage/snapshot_storage.h` | 仅当需要新增诊断接口时才修改；默认不改 |
| `modules/raft/node/raft_node.cpp` | 增加 startup recovery snapshot 候选/选择/replay/post-state 诊断与必要一致性检查 |
| `modules/raft/node/raft_node.h` | 仅当需要测试可观测接口时才修改；默认不改 |
| `tests/persistence_test.cpp` | 补充 meta/restart recovery 与 hard state 边界诊断测试 |
| `tests/test_raft_segment_storage.cpp` | 补充 segment tail truncate 与 meta/log boundary 诊断测试 |
| `tests/test_snapshot_storage_reliability.cpp` | 补充 snapshot validation skip reason 与 invalid snapshot catalog 测试 |
| `tests/test_raft_snapshot_restart.cpp` | 补充 snapshot startup 选择、fallback、tail replay 与 post-state 测试 |
| `tests/test_raft_snapshot_diagnosis.cpp` | 补充 recovery diagnostics 可观测性测试 |
| `tests/snapshot_test.cpp` | 检查并稳定现有 snapshot restart recovery 测试，不删除、不跳过 |

## 7. 不应该修改的文件表

| 文件或区域 | 原因 |
| --- | --- |
| `proto/` | Phase 5 不修改 RPC / Protobuf 契约 |
| `modules/raft/service` | restart recovery diagnostics 不需要修改服务层 |
| `modules/raft/replication` | 不改变 replication 或 catch-up 协议 |
| `modules/raft/state_machine/state_machine.cpp` | 默认不修改 KV 业务语义或 snapshot binary format；仅当诊断需要读取 load error 上下文时再评估 |
| `modules/raft/storage` 持久化格式编码 | Phase 5 不修改 `meta.bin`、segment、snapshot data 或 metadata 格式 |
| CMake / build scripts | Phase 5B 优先扩展现有测试文件，不新增 test target |
| segment log fsync / meta fsync / snapshot atomic publish 实现 | Phase 2-4 已处理 durability barrier；Phase 5 聚焦 recovery validation and diagnostics |

## 8. 需要新增或修改的测试

- meta 缺失、损坏、unsupported version、字段截断和 boundary 不一致测试。
- `log_count`、`first_log_index`、`last_log_index` 与实际 segment 内容不一致的 recovery 失败诊断测试。
- segment tail corruption truncate 的诊断测试，覆盖 partial header、partial payload、checksum mismatch 与后续 segment cleanup。
- snapshot validation skip reason 测试，覆盖 missing meta、missing data、checksum mismatch、staging/temp 目录残留。
- 多 snapshot candidate 测试，验证最新 invalid 回退旧 valid，并能诊断 skipped candidates 与最终选择。
- 全部 snapshot invalid 或无 snapshot 可用时的 startup recovery 诊断测试。
- snapshot load 后 committed tail log replay 的 post-state summary 测试。
- 现有 `RaftSnapshotRecoveryTest.SavesSnapshotAndRestoresAfterRestart` 的稳定性/诊断检查，不删除、不跳过。

## 9. Phase 5B 最小实现计划

1. 在现有测试文件内先补 recovery diagnostics 相关断言，优先覆盖 meta/log/snapshot 三类边界。
2. 在 `raft_storage.cpp` 内补充 `ReadMeta`、`LoadSegments`、`ReadSegmentFile` 的错误上下文和必要 boundary validation，不改变持久化格式。
3. 在 `snapshot_storage.cpp` 内让 snapshot validation skip reason 可记录或可被 node startup 汇总，保持 invalid snapshot 非 fatal、有效 snapshot fallback 语义。
4. 在 `raft_node.cpp` 内补充 startup recovery summary：snapshot candidates、skipped snapshots、selected snapshot、replay range、post-state tuple。
5. 运行 persistence、segment storage、snapshot reliability、snapshot restart、snapshot diagnosis 相关测试。
6. 更新 Phase 5B progress、decisions 与阶段报告。

## 10. 是否修改持久化格式

否。Phase 5A 结论是 Phase 5 不需要修改 `meta.bin`、segment log、snapshot data 或 snapshot metadata 的字段、编码、文件名或目录命名。Phase 5B 应聚焦校验、诊断和测试，不引入格式迁移。

## 11. 跨平台 durability 相关注意事项

Phase 5 不新增 durability barrier，也不应回退 Phase 2-4 的跨平台规则。POSIX 路径继续要求真实 `fsync`；Windows 路径不允许 no-op success，必须使用 `FlushFileBuffers` 或明确返回错误。Phase 5B 若新增任何 required durability operation，必须遵守同一规则；如果未在 Windows 实机或 CI 验证，报告必须明确标记未验证。

## 12. 明确确认没有修改业务代码

Phase 5A 仅更新任务、进度、决策和分析报告。未修改 `.h` / `.cpp` / `.proto` / CMake / 测试代码，未实现 recovery 修改，未修改业务逻辑、Raft 协议、KV 语义或持久化格式。
