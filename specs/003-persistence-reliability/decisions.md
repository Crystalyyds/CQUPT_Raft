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
