# 003-persistence-reliability Decisions

## D001

- Decision: 本阶段只定义 durability contract，不修改代码
- Status: Accepted
- Reason: P0-1 的目标是先把现状能力、缺口和后续修补边界文档化，避免在缺少明确 contract 的情况下直接改高风险持久化路径

## D002

- Decision: 本阶段不修改持久化格式
- Status: Accepted
- Reason: `meta.bin`、segment log、snapshot metadata 和 snapshot data 的格式属于高风险兼容边界，Phase 1 只描述语义，不引入格式迁移

## D003

- Decision: 本阶段不引入新的 storage engine
- Status: Accepted
- Reason: 当前问题是 durability contract 不清晰，不是 storage engine 缺失；先收敛 contract 再决定实现手段

## D004

- Decision: 本阶段不处理 snapshot 性能优化
- Status: Accepted
- Reason: 当前关注点是 snapshot publish 的 durability 和 recovery 可信性，不是吞吐、压缩或 IO 性能

## D005

- Decision: 本阶段不处理 KV 逻辑
- Status: Accepted
- Reason: KV 状态机在本任务中只作为 snapshot 载体和恢复可观测面，不是本轮变更目标

## D006

- Decision: 本阶段允许把“当前行为”和“目标 contract”并列写出，但必须明确区分
- Status: Accepted
- Reason: 当前实现已经具备部分可恢复语义，但并不等价于目标 durability contract；若不显式区分，容易把未来目标误写成现状承诺

## D007

- Decision: 本阶段把“power loss 下未严格保证”作为显式结论
- Status: Accepted
- Reason: 当前代码没有 `fsync`、`fdatasync` 或 directory `fsync`，不能把成功返回表述为 power-loss-safe durability

## D008

- Decision: `Phase 2` 不处理 `meta.bin` / hard state；这些内容推迟到 `Phase 3`
- Status: Accepted
- Reason: `Phase 2` 只收敛 segment log durability，避免一次修改同时跨 `storage log`、`meta hard state` 和 `node core`

## D009

- Decision: `Phase 2` 实现仅落在 `WriteSegments`、segment tail truncate 和 `ReplaceDirectory`，不触碰 `ReplaceFile`
- Status: Accepted
- Reason: 当前 segment log append / truncate / `log/` directory publish 的同步需求可以在 log 文件和目录路径内闭合，不需要越界到 `meta.bin` publish

## D010

- Decision: 之前 `_WIN32` 分支中的 no-op flush 不满足跨平台 durability contract，Phase 2 必须消除这类静默成功路径
- Status: Accepted
- Reason: 如果 Windows 路径在 file flush 或 directory flush 上直接 `return true`，则 contract 只在 POSIX/Linux 上成立，不能称为跨平台 durability 语义

## D011

- Decision: Windows 分支使用 `FlushFileBuffers`；directory flush 使用 `CreateFileW` + `FILE_FLAG_BACKUP_SEMANTICS`，失败时必须返回错误
- Status: Accepted
- Reason: Phase 2 允许范围内只能在 storage helper 内补齐平台语义，不能把 Windows directory flush 继续作为静默 no-op

## D012

- Decision: Windows 路径当前属于 build-level completed / unverified runtime semantics
- Status: Accepted
- Reason: 本轮执行环境不是 Windows；代码路径已补齐，但未在 Windows 实机或 CI 上验证运行时语义

## D013：跨平台 durability 不允许静默降级

背景：

Phase 2 中 segment log durability 实现引入了 POSIX fsync 语义，但 Windows 分支不能用 no-op success 占位，否则会让 durability contract 在不同平台上语义不一致。

决策：

- POSIX/Linux 使用真实 fsync。
- Windows 使用 FlushFileBuffers。
- 如果某个平台无法提供等价保证，必须明确返回错误或记录较弱保证。
- 不允许平台分支静默 no-op 后返回成功。

影响：

后续 Phase 3 的 meta.bin / hard state fsync 语义也必须遵守这个规则。

## D014

- Decision: Phase 3A 只输出 `meta.bin` / hard state 的 affected-files plan 和测试设计，不修改业务代码
- Status: Accepted
- Reason: 当前目标是先把 `WriteMeta`、`ReplaceFile`、`PersistStateLocked` 的职责边界、测试场景和最小实现范围固定下来，避免在高风险 `node` / `storage` 边界上直接实现

## D015

- Decision: Phase 3A 的测试设计优先复用现有 `tests/test_raft_segment_storage.cpp` 与 `tests/persistence_test.cpp`
- Status: Accepted
- Reason: 本阶段只做计划与测试设计，不创建 task 子目录，也不预先引入新的测试 target；优先在已有 storage / persistence 测试面上扩展 `meta.bin` 与 hard state 场景

## D016

- Decision: Phase 3A 不扩大到 snapshot publish、segment log publish 或持久化格式修改
- Status: Accepted
- Reason: `meta.bin` / hard state durability 已经可以在 `WriteMeta`、`ReplaceFile`、`PersistStateLocked` 三个入口内闭合，当前没有发现必须越界到 Phase 2 或 Phase 4 的阻塞项

## D017

- Decision: Phase 3B 只处理 `meta.bin` / hard state 持久化语义，不处理 segment log、snapshot、KV、proto、RPC 或持久化格式变更
- Status: Accepted
- Reason: Phase 2 已经处理 segment log durability；Phase 3B 聚焦 hard state durability，避免一次修改跨多个持久化子系统

## D018

- Decision: Phase 3B 的最小实现只落在 `WriteMeta`、`ReplaceFile` 和 `PersistStateLocked`
- Status: Accepted
- Reason: `meta.bin` / hard state 的 durability 语义可以在这三个入口内闭合，不需要触碰 `WriteSegments`、`ReplaceDirectory`、snapshot publish 或持久化格式

## D019

- Decision: Phase 3B 继续复用 Phase 2 的跨平台 flush helper 约束，Windows 路径不允许出现新的 no-op success
- Status: Accepted
- Reason: `meta.bin` file / directory durability 与 segment log durability 共享同一跨平台 contract；如果 Phase 3B 在 Windows 路径退化为 no-op，则 hard state durability 语义会再次按平台分裂

## D020

- Decision: Phase 4 拆分为 `Phase 4A` 分析与任务生成，以及 `Phase 4B` 最小实现
- Status: Accepted
- Reason: snapshot publish 同时跨 `snapshot_storage`、`state_machine` 和 `raft_node` 编排边界；先固定 current path、crash window 和 trusted-state 规则，再实现原子发布，可以避免在高风险恢复路径上直接改动

## D021

- Decision: Phase 4B 的主实现面优先收敛在 `modules/raft/storage/snapshot_storage.cpp`，`raft_node.cpp` 与 `state_machine.cpp` 仅在证明存在 publish ordering 或失败传播缺口时最小触碰
- Status: Accepted
- Reason: 当前 direct-to-final-dir publish、`remove_all(final_dir)` 先删后发窗口、snapshot data/meta 缺少 `fsync` / directory `fsync` 都集中在 snapshot catalog 路径；node 与 state machine 主要承担编排和工作文件边界，默认不扩大改动

## D022

- Decision: Phase 4B 不修改 snapshot data / metadata 持久化格式，只收敛 snapshot atomic publish 语义
- Status: Accepted
- Reason: 当前问题是 publish protocol 与 durability barrier 缺失，不是 `data.bin` 或 `__raft_snapshot_meta` 编码不够；修改格式会引入兼容性和恢复迁移风险，超出本阶段目标

## D023

- Decision: Phase 4B 必须消除 snapshot direct-to-final-dir publish 和 `_WIN32` 静默成功式 durability 降级
- Status: Accepted
- Reason: 现有 `SaveSnapshotFile()` 直接写最终 `snapshot_<index>/` 目录，且目录 create/remove/publish 没有 durability barrier；如果 Phase 4B 仍保留 direct publish 或某个平台继续 no-op success，就无法满足跨平台 snapshot atomic publish contract

## D024

- Decision: Phase 4B 对同 index、同 term 的已有有效 snapshot 采用幂等成功，不删除并重写最终目录
- Status: Accepted
- Reason: 跨平台文件系统无法可靠提供“非空目录原子替换”的统一语义；对已经可信的同边界 snapshot 执行幂等返回，可以消除 `remove_all(final_dir)` 先删旧 snapshot 再发布新 snapshot 的 crash window

## D025

- Decision: Phase 4B 对同 index 但 term 不一致的已有有效 snapshot 返回明确错误，不进行目录替换
- Status: Accepted
- Reason: 同 index 不同 term 的 snapshot 边界不应被静默覆盖；在没有跨平台非空目录原子交换能力的前提下，显式失败比删除可信旧 snapshot 更符合 durability contract

## D026

- Decision: Phase 4B 暂不修改 `raft_node.cpp` 或 `state_machine.cpp`
- Status: Accepted
- Reason: 当前最小实现已在 `snapshot_storage.cpp` 内闭合 staged publish、snapshot data/meta file fsync、directory fsync 和 recovery 过滤规则；现有 snapshot / restart / diagnosis 测试未暴露 `SnapshotWorkerLoop`、`OnInstallSnapshot` 或 state machine 工作文件边界必须扩大修改

## D027

- Decision: Phase 4B 为 prune 删除路径补齐 `snapshot_dir` directory fsync，但不重写 prune 策略
- Status: Accepted
- Reason: prune 删除旧 snapshot 后的目录项 durability 可以通过删除后同步 `snapshot_dir` 收敛，不需要引入新的 prune 状态机、墓碑文件或持久化格式

## D028

- Decision: Phase 4B Windows snapshot flush 路径沿用 `FlushFileBuffers` 与 directory handle 方案，运行时语义未在 Windows 环境验证
- Status: Accepted
- Reason: 本轮执行环境是 POSIX/Linux；Windows 分支没有 no-op success，失败会返回错误，但没有 Windows 实机或 CI 结果，必须继续标记为未验证
