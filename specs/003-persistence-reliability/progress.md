# 003-persistence-reliability Progress

## Current Phase

- `Phase 3B: meta hard state fsync minimal implementation`

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

## In Progress

- `None`

## Blocked

- `None`

## Next

- `Phase 4 entry: snapshot publish affected-files plan`

## Notes

- Phase 3B 已完成最小实现与回归验证
- Phase 3B 只修改 `WriteMeta`、`ReplaceFile`、`PersistStateLocked` 及相关测试
- 本阶段不修改持久化格式
- 未修改 snapshot / KV / proto / transport
- 未修改 segment log append / truncate 逻辑
- Windows 路径沿用已有 contract，但 Phase 3B 未在 Windows 环境验证
- 结论来自当前源码与代表性测试现状，不是新语义变更
