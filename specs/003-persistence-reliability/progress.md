# 003-persistence-reliability Progress

## Current Phase

- `Phase 6B: crash / failure injection tests minimal implementation`

## Completed

- 已完成根 `AGENTS.md` 与 `storage` / `node` / `state_machine` 模块 `AGENTS.md` 阅读
- 已完成 `raft_storage`、`snapshot_storage`、`raft_node`、`state_machine` 持久化与恢复路径勘察
- 已完成 persistence / snapshot / restart recovery 代表性测试阅读
- 已完成 T201：segment log append durability 边界实现
- 已完成 T202：segment log truncate durability 边界实现
- 已完成 T203：`log/` directory publish 边界实现
- 已完成 T204：`WriteSegments` 受影响点实现
- 已完成 T205：`ReplaceDirectory` 受影响点实现
- 已完成 T206：segment log append / truncate / recovery 测试补充与通过
- 已完成 Windows flush helper 修正，移除 `_WIN32` no-op 静默成功分支
- 已完成 T301：`meta.bin` 当前写入与恢复路径梳理
- 已完成 T302：hard state file `fsync` / directory `fsync` 需求梳理
- 已完成 T303：`meta.bin` publish ordering 风险梳理
- 已完成 T304：`WriteMeta` 受影响点规划
- 已完成 T305：`ReplaceFile` 受影响点规划
- 已完成 T306：`PersistStateLocked` durable publish 边界规划
- 已完成 T307：Phase 3A affected-files plan 与测试设计报告
- 已完成 T308：补充 meta.bin / hard state restart recovery 相关测试
- 已完成 T309：`WriteMeta` file fsync 语义实现
- 已完成 T310：`ReplaceFile` meta.bin publish / replace 后 directory fsync 语义实现
- 已完成 T311：`PersistStateLocked` hard state 持久化失败传播检查与修正
- 已完成 T312：确认 POSIX 分支使用真实 fsync
- 已完成 T313：确认 Windows 分支不使用 no-op success
- 已完成 T314：确认未修改 `meta.bin` 持久化格式
- 已完成 T315：运行 persistence / restart recovery 相关测试
- 已完成 T316：更新 Phase 3B 进度状态
- 已完成 T317：更新 Phase 3B 决策记录
- 已完成 T318：生成 Phase 3B 阶段报告
- 已完成 T401：本地 snapshot save / publish 调用链梳理
- 已完成 T402：`InstallSnapshot` 持久化 / 加载 / 恢复调用链梳理
- 已完成 T403：startup snapshot load / trusted-state 选择规则梳理
- 已完成 T404：snapshot data / metadata 与 state machine work-file 边界梳理
- 已完成 T405：snapshot file `fsync` / directory `fsync` 缺口梳理
- 已完成 T406：snapshot publish crash window 与 log compaction 边界梳理
- 已完成 T407：Phase 4B 受影响文件与测试范围规划
- 已完成 T408：Phase 4A snapshot atomic publish 分析报告
- 已完成 T409：snapshot publish 从 direct-to-final-dir 改为 staged temp snapshot dir publish
- 已完成 T410：`snapshot_<index>/data.bin` file fsync 语义实现
- 已完成 T411：`snapshot_<index>/__raft_snapshot_meta` file fsync 语义实现
- 已完成 T412：staged snapshot dir、`snapshot_dir` publish 和 prune 路径 directory fsync 语义实现
- 已完成 T413：同 index snapshot 覆盖路径改为有效 snapshot 幂等处理，避免先删后发窗口
- 已完成 T414：验证 `SnapshotWorkerLoop` / `OnInstallSnapshot` 失败暴露与时序边界，未修改 `raft_node.cpp`
- 已完成 T415：确认 restart recovery 继续忽略 temp / incomplete snapshot，并保持有效 snapshot 回退规则
- 已完成 T416：更新 snapshot storage reliability 测试
- 已完成 T417：检查 snapshot / restart / diagnosis 相关断言，现有断言无需源码修改
- 已完成 T418：确认 POSIX 使用真实 fsync，Windows 路径不使用 no-op success
- 已完成 T419：确认未修改 snapshot data / metadata 持久化格式
- 已完成 T420：运行 snapshot / restart / recovery 与 persistence 相关测试
- 已完成 T421：更新 Phase 4B 进度状态
- 已完成 T422：更新 Phase 4B 决策记录
- 已完成 T423：生成 Phase 4B 阶段报告
- 已完成 T501：restart recovery 整体调用链梳理
- 已完成 T502：`meta.bin` 加载、校验、失败处理与诊断现状梳理
- 已完成 T503：segment log 加载、校验、tail truncate 与诊断现状梳理
- 已完成 T504：snapshot 枚举、校验、选择、加载与 skip/fallback 诊断现状梳理
- 已完成 T505：meta / log / snapshot trusted-state 边界关系梳理
- 已完成 T506：recovery 后关键 Raft 状态设置与一致性风险梳理
- 已完成 T507：restart recovery 相关测试覆盖与缺口梳理
- 已完成 T508：Phase 5B 受影响文件与测试范围规划
- 已完成 T509：更新 Phase 5A 进度与决策记录
- 已完成 T510：生成 Phase 5A restart recovery diagnostics 分析报告
- 已完成 T511：`ReadMeta` / `LoadSegmented` / `LoadSegments` 增加 path、字段名、meta boundary 与 segment boundary 诊断上下文
- 已完成 T512：明确并测试 `meta.bin` log boundary invariant
- 已完成 T513：segment tail corruption truncate 增加原因、path、offset、保留记录数和后续 segment 清理诊断
- 已完成 T514：snapshot catalog validation 暴露 skip reason
- 已完成 T515：startup snapshot recovery 增加候选数量、跳过条目、选择结果和无可用 snapshot 诊断
- 已完成 T516：recovery 后增加 commit/apply/snapshot/log 状态摘要与 replay failure 上下文
- 已完成 T517：补充 meta unsupported version、boundary 不一致和 commit/apply clamp 测试
- 已完成 T518：补充 snapshot validation issue 测试，覆盖 staging、缺失 meta、缺失 data、checksum mismatch
- 已完成 T519：补充 earlier segment tail truncate 与后续 segment 清理测试
- 已完成 T520：检查 snapshot restart recovery 相关测试，未删除或跳过测试
- 已完成 T521：运行 persistence / segment storage / snapshot restart / snapshot diagnosis 相关测试
- 已完成 T522：更新 Phase 5B 进度状态
- 已完成 T523：更新 Phase 5B 决策记录
- 已完成 T524：生成 Phase 5B 阶段报告
- 已完成 T601：分析当前 persistence / snapshot / restart recovery 测试覆盖
- 已完成 T602：识别 meta、segment log、snapshot publish 和 restart recovery 关键 crash window
- 已完成 T603：区分可通过坏文件 / 坏目录构造完成的测试场景
- 已完成 T604：区分必须依赖 test-only failure injection 的测试场景
- 已完成 T605：识别 fsync、directory fsync、rename / replace、remove / prune、partial write 失败模拟缺口
- 已完成 T606：判断需要新增 crash matrix 文档或章节
- 已完成 T607：规划 Phase 6B 最小测试实现范围
- 已完成 T608：更新 Phase 6A 进度与决策记录
- 已完成 T609：生成 Phase 6A crash / failure injection 分析报告
- 已完成 T610：在 Phase 6B 报告中补充 crash matrix
- 已完成 T611：补充 segment/meta/log 文件与目录构造类 crash artifact 测试
- 已完成 T612：补充 snapshot 全部 invalid 时无可信 snapshot 的测试
- 已完成 T617：补充 meta/log publish window 的 restart recovery trusted-state 测试
- 已完成 T618：确认本阶段未引入 failure injection hook，未改变生产路径语义或持久化格式
- 已完成 T619：运行 persistence / segment storage / snapshot reliability / snapshot restart / snapshot diagnosis 相关测试
- 已完成 T620：更新 Phase 6B 进度状态
- 已完成 T621：更新 Phase 6B 决策记录
- 已完成 T622：生成 Phase 6B 阶段报告

## In Progress

- `None`

## Blocked

- T613-T616：精确 `fsync`、directory `fsync`、rename / replace、remove / prune、partial write failure injection 仍需要 test-only hook；Phase 6B 未修改生产代码，因此未实现该 hook

## Next

- `Phase 6C: test-only failure injection hook design / implementation, if approved`

## Notes

- Phase 3B 已完成最小实现与回归验证
- Phase 3B 只修改 `WriteMeta`、`ReplaceFile`、`PersistStateLocked` 及相关测试
- Phase 4A 已确认当前 snapshot publish 直接写最终 `snapshot_<index>/` 目录，没有 temp snapshot dir
- Phase 4A 已确认 trusted snapshot 规则是“最新有效优先；无效跳过；允许回退到更旧有效 snapshot”
- Phase 4B 已完成 staged snapshot publish；主实现面在 `snapshot_storage.cpp`
- Phase 4B 未修改 `raft_node.cpp` 或 `state_machine.cpp`，当前测试未证明需要扩大到编排层
- Phase 5A 已确认 restart recovery 的核心路径是先恢复 `meta.bin + log/`，再选择并加载 snapshot，最后 replay committed tail log
- Phase 5A 已确认当前 trusted-state 规则存在，但诊断不足：snapshot skip reason、segment truncate reason、meta/log boundary mismatch 和 recovery 后状态摘要不够清晰
- Phase 5A 只做分析、任务补充和状态文档更新，未修改业务代码
- Phase 5B 已完成 restart recovery 校验、诊断和测试补强
- Phase 5B 增加了 snapshot validation diagnostics 的头文件声明；这是为 `ISnapshotStorage` 暴露 skip reason 的最小接口扩展，不改变 snapshot 持久化格式
- Phase 5B 未修改 segment log fsync、meta fsync 或 snapshot atomic publish 语义
- Phase 5B 未修改 proto / RPC / KV / transport
- Phase 5B 相关测试在当前 POSIX/Linux 环境通过
- Phase 6A 已确认现有测试主要覆盖构造坏文件 / 坏目录后的恢复行为，尚未覆盖持久化操作中间失败
- Phase 6A 已确认当前没有精确 crash point / fsync failure / rename failure / prune failure 注入能力
- Phase 6A 建议 Phase 6B 先补文件构造类测试，再引入默认关闭的 test-only failure injection helper
- Phase 6A 只做分析、任务补充和状态文档更新，未修改业务代码或测试代码
- Phase 6B 已优先补充坏文件 / 坏目录 / temp 残留 / checksum 损坏类测试，没有引入复杂 mock 文件系统
- Phase 6B 新增测试覆盖 partial segment header、`meta.bin.tmp` / `log.tmp` / `log.bak` 残留、old meta + new log、新 meta + old log、全部 invalid snapshot
- Phase 6B 未修改生产 `.h` / `.cpp`，未引入 test-only failure injection hook
- Phase 6B 未覆盖精确 fsync、directory fsync、rename、remove、prune 和 partial write 注入失败；这些需要后续 hook 设计
- Phase 6B 相关测试在当前 POSIX/Linux 环境通过
- 本阶段不修改持久化格式
- 未修改 KV / proto / RPC / transport
- 未修改 segment log append / truncate 逻辑
- Windows 路径沿用已有 contract，但 Phase 4 相关路径当前仍未在 Windows 环境验证
- 结论来自当前源码与代表性测试现状，不是新语义变更
