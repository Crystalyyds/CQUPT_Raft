# CONSISTENCY_LAYER_TRANSITION_PLAN

> 本文基于 `docs/CURRENT_INDUSTRIALIZATION_ANALYSIS.md` 的结论制定，不重复展开大量现状分析。默认前提是：当前项目已有较完整的 Raft 主流程验证能力，但距离工业级 consistency layer 还缺 durability、抽象边界、故障模型与可观测性。

## 1. 转型目标

项目后续目标不是继续演化成完整 KV 数据库，而是从当前 KV demo 型 Raft 项目，逐步演进为：

- Raft core
- replicated log substrate
- persistence layer
- snapshot / recovery subsystem
- membership / configuration boundary
- state machine abstraction
- transport boundary
- failure recovery and diagnostics foundation

KV 的未来定位：

- 保留为 demo
- 保留为 smoke test surface
- 保留为 sample state machine
- 不再作为最终产品核心方向

最终核心产品能力应集中在：

- 共识与复制
- 持久化与恢复
- 快照与追赶
- 一致性层接口与扩展点

---

## 2. 目标架构方向

### Raft core 应该提供什么能力

Raft core 最终应成为一个不感知具体 KV 业务的共识协调器，负责：

- election / term / vote
- replicated log progress
- majority commit
- apply boundary推进
- leader/follower/candidate 生命周期
- snapshot install 和 compaction 协调
- membership 变更入口
- 基础 invariants 和状态导出

Raft core 不应继续承担：

- KV key/value 校验
- KV request limits
- KV 业务状态读取
- demo client 语义

### replicated log substrate 应该提供什么能力

它应成为一致性层的核心产品面之一，至少应提供：

- append proposal entry
- query by index / term
- conflict-aware follower catch-up
- commit watermark
- applied watermark
- log truncate / compact
- storage-backed recovery
- future entry type extension

从当前代码看，这部分能力散落在 `node`、`replication`、`storage` 中，后续应该逐步被明确成一个独立可描述的子系统，而不是继续仅依靠 `RaftNode` 内部方法组合。

### state machine abstraction 应该长什么样

方向上应从当前 `IStateMachine` 继续演进，但目标不是更复杂的 KV API，而是更清晰的一致性层扩展边界。

它最终应回答：

- 日志项如何交给上层状态机
- snapshot 如何由状态机生成
- restore 如何把 snapshot 恢复回上层引擎
- 状态机是否需要批应用、幂等约束、版本边界
- 状态机和 replicated log 的交界是什么

当前的 `Apply/SaveSnapshot/LoadSnapshot` 是足够好的起点，但仍需要去 KV 化和规范化。

### snapshot provider / restore interface 应该承担什么职责

建议方向：

- state machine 负责“业务状态快照内容”
- snapshot subsystem 负责“快照发布、枚举、校验、保留策略”
- Raft core 负责“何时触发快照、何时安装快照、如何更新 log 边界”

也就是把“生成内容”“保存工件”“共识协调”三层职责继续拉开。

### storage engine 应该如何接入

未来不是让 storage engine 直接改 Raft 内部，而是通过明确接口接入：

- proposal entry format / payload boundary
- state machine apply contract
- snapshot export/import contract
- durability/reporting contract

KV 只是第一个 sample engine。

### transport / RPC 边界应该如何划分

目标方向：

- Raft internal transport boundary 独立于 demo/client API
- client/demo API 作为上层示例接口
- internal RPC error taxonomy、retry、timeout、peer unavailable 语义清晰

也就是说：

- `RaftService` 应朝“一致性层内部协议”发展
- `KvService` 应逐步被定位为 sample app interface，而不是核心产品 contract

### KV 未来应该处于什么位置

KV 应转型为：

- `examples/` 风格样例状态机思维模型
- smoke test state machine
- integration verification surface

而不是：

- Raft core 的需求来源
- 节点配置模型的中心
- 公共接口设计的主导者

---

## 3. 转型原则

1. 小步提交
2. 每阶段可验证
3. 不混合行为变更和结构重构
4. 不一次性大改
5. 先稳定 Raft core，再抽象 storage engine
6. 先补测试和验收标准，再逐步实现
7. KV 先保留为验证层，不急于删除
8. 每阶段结束必须能构建和测试
9. 高风险特性按依赖顺序推进，不倒序推进
10. 所有阶段都优先保住已有 recovery 和 snapshot 能力

---

## 4. 分阶段路线

### Phase 1: baseline stabilization

#### 目标

- 明确当前系统基线能力
- 明确构建、测试、单节点启动方式
- 明确 KV 是 demo，不是产品方向
- 明确哪些模块是 core，哪些模块是 sample/service/app

#### 非目标

- 不改核心行为
- 不改协议
- 不改持久化格式
- 不做逻辑重构

#### 涉及模块

- 根目录 `AGENTS.md`
- `apps`
- `proto`
- `modules/raft/node`
- `modules/raft/storage`
- `modules/raft/replication`
- `modules/raft/state_machine`
- `tests`

#### 需要阅读的 AGENTS.md

- `/AGENTS.md`
- `apps/AGENTS.md`
- `proto/AGENTS.md`
- `modules/raft/node/AGENTS.md`
- `modules/raft/storage/AGENTS.md`
- `modules/raft/replication/AGENTS.md`
- `modules/raft/state_machine/AGENTS.md`

#### 主要任务

- 形成 baseline docs
- 形成 current industrialization analysis
- 形成 transition plan
- 明确 KV demo 定位
- 明确 core/demo 边界
- 确认构建与测试入口

#### 需要新增或修改的测试类型

- 无实现
- 只定义后续补测方向

#### 验收标准

- 文档能清晰描述现状、边界和转型方向
- 后续每个 Phase 都有可承接的起点

#### 风险

- 如果这一步界定不清，后续会把 KV 和 consistency layer 混着演进

#### 完成后预计成熟度提升

- `58 -> 60`
- 主要提升来自路线清晰，不来自行为增强

### Phase 2: Raft core correctness baseline

#### 目标

- 明确 election、term、vote、commit、apply 的 correctness baseline
- 把 safety invariants 显式化
- 建立“核心共识逻辑的验收标准”

#### 非目标

- 不先做 membership change
- 不先做 transport 替换

#### 涉及模块

- `modules/raft/node`
- `modules/raft/replication`
- `proto`
- `tests`

#### 需要阅读的 AGENTS.md

- `/AGENTS.md`
- `modules/raft/node/AGENTS.md`
- `modules/raft/replication/AGENTS.md`
- `proto/AGENTS.md`

#### 主要任务

- 整理 invariants 文档
- 分类现有测试与缺口
- 设计 correctness-oriented test plan
- 明确 leader completeness、log matching、majority commit、split-brain protection 的验收描述

#### 需要新增或修改的测试类型

- correctness case matrix
- deterministic scenario plan
- stale term / stale vote / conflict hint 扩展计划

#### 验收标准

- 核心 Raft 安全性边界被明确成文
- 能说清哪些行为已经验证，哪些仍待补

#### 风险

- 如果没有 invariant baseline，后续任何结构抽象都会带着 correctness 不确定性

#### 完成后预计成熟度提升

- `60 -> 65`

### Phase 3: replicated log and persistence reliability

#### 目标

- 明确 replicated log 子系统边界
- 明确 durability 语义和 crash recovery 语义
- 为后续真正的 consistency substrate 打基础

#### 非目标

- 不先做复杂性能优化
- 不先换存储引擎

#### 涉及模块

- `modules/raft/storage`
- `modules/raft/node`
- `modules/raft/replication`
- `tests`

#### 需要阅读的 AGENTS.md

- `/AGENTS.md`
- `modules/raft/storage/AGENTS.md`
- `modules/raft/node/AGENTS.md`
- `modules/raft/replication/AGENTS.md`

#### 主要任务

- 明确 hard state durability spec
- 明确 segment append / replace / truncate spec
- 明确 partial write / checksum / crash windows
- 设计 crash matrix 和 persistence validation plan

#### 需要新增或修改的测试类型

- crash point tests
- disk corruption tests
- segment/meta consistency tests
- recovery matrix tests

#### 验收标准

- 能清楚回答“什么情况下数据一定保存”“什么情况下可能丢但不会破坏 safety”
- 有 persistence readiness checklist

#### 风险

- 这是最容易误触数据丢失边界的阶段

#### 完成后预计成熟度提升

- `65 -> 71`

### Phase 4: state machine abstraction

#### 目标

- 把 KV 从核心一致性层中降级为 demo state machine
- 明确 state machine 接入规范
- 让 future storage engine 接入有清晰入口

#### 非目标

- 不删除 KV
- 不直接实现新的真实 storage engine

#### 涉及模块

- `modules/raft/state_machine`
- `modules/raft/node`
- `modules/raft/common`
- `modules/raft/service`

#### 需要阅读的 AGENTS.md

- `/AGENTS.md`
- `modules/raft/state_machine/AGENTS.md`
- `modules/raft/node/AGENTS.md`
- `modules/raft/common/AGENTS.md`
- `modules/raft/service/AGENTS.md`

#### 主要任务

- 明确 state machine contract
- 标注 KV-specific boundary
- 设计 sample state machine vs substrate interface 的分离方案
- 识别 `KvRequestLimits`、`DebugGetValue`、KV request validation 等耦合点

#### 需要新增或修改的测试类型

- state machine abstraction contract tests
- alternate mock state machine tests
- snapshot/restore contract tests

#### 验收标准

- 能明确说出“删除 KV 业务语义后，Raft core 剩下什么”
- KV 成为 sample，而不是 core design center

#### 风险

- 容易把抽象设计做成“更复杂的 KV API”

#### 完成后预计成熟度提升

- `71 -> 75`

### Phase 5: snapshot and log compaction

#### 目标

- 工业化 snapshot save/load/install/restore/compaction 语义
- 明确 snapshot publish 和 log compaction 的协作边界

#### 非目标

- 不做大型 snapshot pipeline 优化

#### 涉及模块

- `modules/raft/node`
- `modules/raft/storage`
- `modules/raft/state_machine`
- `modules/raft/replication`

#### 需要阅读的 AGENTS.md

- `/AGENTS.md`
- `modules/raft/node/AGENTS.md`
- `modules/raft/storage/AGENTS.md`
- `modules/raft/state_machine/AGENTS.md`
- `modules/raft/replication/AGENTS.md`

#### 主要任务

- 明确 snapshot atomicity spec
- 明确 install snapshot interrupted/retry/restart 行为
- 明确 compaction boundary invariant
- 明确 lagging follower snapshot catch-up acceptance

#### 需要新增或修改的测试类型

- interrupted snapshot publish tests
- snapshot install crash/restart tests
- snapshot/log boundary invariant tests

#### 验收标准

- snapshot/recovery 路径具备可解释、可验证、可回归的规范

#### 风险

- 这是 safety 与 durability 双高风险区域

#### 完成后预计成熟度提升

- `75 -> 79`

### Phase 6: transport and RPC boundary

#### 目标

- 区分 internal Raft RPC 和 client/demo RPC
- 明确 timeout、retry、error taxonomy
- 为 transport abstraction 铺路

#### 非目标

- 不强制当阶段替换 gRPC

#### 涉及模块

- `proto`
- `modules/raft/service`
- `modules/raft/node`
- `apps`

#### 需要阅读的 AGENTS.md

- `/AGENTS.md`
- `proto/AGENTS.md`
- `modules/raft/service/AGENTS.md`
- `modules/raft/node/AGENTS.md`
- `apps/AGENTS.md`

#### 主要任务

- 把 internal 与 demo/client contract 分层描述清楚
- 设计 transport adapter boundary
- 设计 unavailable peer / retry / timeout contract
- 明确 leader redirect 的 sample/demo 定位

#### 需要新增或修改的测试类型

- transport error taxonomy tests
- unavailable peer tests
- retry/backoff behavior tests

#### 验收标准

- 能在不改 core 算法语义的前提下描述 transport 替换路径

#### 风险

- 如果过早同时替换 transport 和 core，会放大排障成本

#### 完成后预计成熟度提升

- `79 -> 82`

### Phase 7: membership and cluster configuration

#### 目标

- 工业化 static membership 边界
- 为 future dynamic membership 预留接口和文档

#### 非目标

- 不要求本阶段实现 joint consensus

#### 涉及模块

- `modules/raft/common`
- `modules/raft/node`
- `apps`
- `proto`

#### 需要阅读的 AGENTS.md

- `/AGENTS.md`
- `modules/raft/common/AGENTS.md`
- `modules/raft/node/AGENTS.md`
- `apps/AGENTS.md`
- `proto/AGENTS.md`

#### 主要任务

- 明确 static membership contract
- 明确 identity / config validation baseline
- 设计 membership change roadmap 入口

#### 需要新增或修改的测试类型

- static config validation tests
- identity misuse tests
- membership preflight tests

#### 验收标准

- static membership 成为“清晰受限能力”，而不是隐式假设

#### 风险

- 如果这一步跳过，后续实现 membership change 会缺乏稳定落点

#### 完成后预计成熟度提升

- `82 -> 84`

### Phase 8: concurrency and lifecycle hardening

#### 目标

- 明确 node start/stop/restart lifecycle
- 梳理 timer、thread pool、callback lifetime、lock order 风险

#### 非目标

- 不做大范围性能调优

#### 涉及模块

- `modules/raft/node`
- `modules/raft/runtime`
- `modules/raft/replication`
- `modules/raft/service`

#### 需要阅读的 AGENTS.md

- `/AGENTS.md`
- `modules/raft/node/AGENTS.md`
- `modules/raft/runtime/AGENTS.md`
- `modules/raft/replication/AGENTS.md`
- `modules/raft/service/AGENTS.md`

#### 主要任务

- lifecycle 状态建模
- lock hierarchy 文档化
- callback shutdown 边界文档化
- data race review plan

#### 需要新增或修改的测试类型

- shutdown stress tests
- repeated start/stop tests
- TSAN-oriented test plan

#### 验收标准

- 生命周期边界不再只依靠代码阅读理解

#### 风险

- 这是最容易引入隐蔽回归的阶段之一

#### 完成后预计成熟度提升

- `84 -> 87`

### Phase 9: observability and diagnostics

#### 目标

- 让一致性层具备可运维性
- 强化恢复、复制、snapshot、持久化故障诊断

#### 非目标

- 不要求一次性接入完整平台级监控栈

#### 涉及模块

- `modules/raft/runtime`
- `modules/raft/node`
- `modules/raft/service`
- `apps`

#### 需要阅读的 AGENTS.md

- `/AGENTS.md`
- `modules/raft/runtime/AGENTS.md`
- `modules/raft/node/AGENTS.md`
- `modules/raft/service/AGENTS.md`
- `apps/AGENTS.md`

#### 主要任务

- 定义状态导出与诊断字段
- 定义 snapshot/recovery diagnostic events
- 定义 replication progress diagnostics
- 设计 trace/correlation id 方案

#### 需要新增或修改的测试类型

- diagnostic completeness tests
- metric export consistency tests

#### 验收标准

- 线上问题能更快区分：transport、durability、snapshot、replay、quorum、peer lag

#### 风险

- 如果没有统一事件模型，只加日志会继续堆积噪声

#### 完成后预计成熟度提升

- `87 -> 90`

### Phase 10: failure injection and distributed storage readiness

#### 目标

- 把项目从“工程化原型”推进到“面向真实分布式存储接入的 readiness 阶段”

#### 非目标

- 不要求当阶段直接实现完整产品化存储服务

#### 涉及模块

- 全模块
- 尤其 `node`、`replication`、`storage`、`state_machine`、`tests`

#### 需要阅读的 AGENTS.md

- `/AGENTS.md`
- 各核心模块 AGENTS.md

#### 主要任务

- crash simulation
- network partition injection
- disk error injection
- deterministic integration framework
- performance smoke baseline

#### 需要新增或修改的测试类型

- fault injection suite
- deterministic cluster simulation
- restart-after-fault suite
- performance smoke suite

#### 验收标准

- 项目可以被评估为“适合接入更真实的上层存储引擎做下一阶段演进”

#### 风险

- 如果前面各阶段未形成清晰 spec，这一阶段会变成“复杂但不可解释的随机测试堆积”

#### 完成后预计成熟度提升

- `90 -> 93`

---

## 5. 第一阶段详细框架

### Phase 1 spec 草案

标题建议：

- `Baseline Stabilization for Raft Consistency-Layer Transition`

应包含：

- 当前项目目标与非目标
- KV 的 sample/demo 定位
- core modules vs demo/app/service modules 划分
- build/test/start 基线
- 后续 phase 编号和承接关系

### Phase 1 plan 草案

1. 读取根 `AGENTS.md`
2. 读取核心模块 `AGENTS.md`
3. 确认构建入口
4. 确认测试入口
5. 确认单节点和静态三节点启动方式
6. 输出现状工业化分析
7. 输出 consistency-layer 转型路线

### Phase 1 tasks 草案

- task 1: inventory current modules and boundaries
- task 2: classify core vs demo
- task 3: record build/test/start commands
- task 4: record current maturity and risks
- task 5: define transition phases

### Phase 1 验收标准

- 能清楚回答：
  - 当前系统是什么
  - 当前系统不是什么
  - KV 在当前系统里扮演什么角色
  - 哪些模块是未来一致性层核心
  - 后续从哪个 Phase 开始实施

### Phase 1 不做事项

- 不修改业务代码
- 不修改协议
- 不修改持久化格式
- 不做结构重构
- 不新增 specs 目录
- 不实现后续任何 phase

---

## 6. 后续 specs 目录建议

只给建议，不在本次创建：

- `specs/001-baseline-stabilization`
- `specs/002-raft-core-correctness`
- `specs/003-replicated-log-persistence`
- `specs/004-state-machine-abstraction`
- `specs/005-snapshot-log-compaction`
- `specs/006-transport-rpc-boundary`
- `specs/007-membership-configuration`
- `specs/008-concurrency-lifecycle`
- `specs/009-observability-diagnostics`
- `specs/010-failure-injection-readiness`

---

## 建议起点

建议下一步从 **Phase 1: baseline stabilization** 开始，并立即把本次两份文档当作该 Phase 的输入基线。

原因：

- 当前项目最缺的不是“再多一个功能点”
- 而是“把现有能力、边界、风险和转型方向先固化成清晰共识”

只有先完成这一步，后续 Phase 2 到 Phase 10 才不会继续把 KV demo 和 consistency layer 混着演进。
