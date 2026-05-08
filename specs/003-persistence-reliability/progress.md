# 003-persistence-reliability Progress

## Current Phase

- `Phase 4B: snapshot atomic publish minimal implementation`

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

## In Progress

- `None`

## Blocked

- `None`

## Next

- `Phase 5: restart recovery validation and diagnostics`

## Notes

- Phase 3B 已完成最小实现与回归验证
- Phase 3B 只修改 `WriteMeta`、`ReplaceFile`、`PersistStateLocked` 及相关测试
- Phase 4A 已确认当前 snapshot publish 直接写最终 `snapshot_<index>/` 目录，没有 temp snapshot dir
- Phase 4A 已确认 trusted snapshot 规则是“最新有效优先；无效跳过；允许回退到更旧有效 snapshot”
- Phase 4B 已完成 staged snapshot publish；主实现面在 `snapshot_storage.cpp`
- Phase 4B 未修改 `raft_node.cpp` 或 `state_machine.cpp`，当前测试未证明需要扩大到编排层
- 本阶段不修改持久化格式
- 未修改 KV / proto / RPC / transport
- 未修改 segment log append / truncate 逻辑
- Windows 路径沿用已有 contract，但 Phase 4 相关路径当前仍未在 Windows 环境验证
- 结论来自当前源码与代表性测试现状，不是新语义变更
