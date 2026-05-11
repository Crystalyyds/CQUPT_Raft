# Implementation Plan: CQUPT_Raft Industrialization Hardening

**Branch**: `[spec-kit]` | **Date**: 2026-05-11 | **Spec**: [spec.md](./spec.md)  
**Input**: Feature specification from `/specs/004-raft-industrialization/spec.md`

## Build and Test Resource Limits
This project is developed on a local machine with limited CPU resources. Do not run high-parallel build or test commands.
Required build commands:

```bash
  cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
  cmake --build build --parallel 2
```


## Summary

本计划基于当前 CQUPT_Raft 已有实现推进工业化补强，而不是重新设计
Raft。现有稳定能力保持为受保护基线，`003-persistence-reliability`
已完成的 durability hardening 视为既有成果。当前计划只处理剩余缺口：
Linux 主验证路径中的 flaky 阻塞、精确 failure injection、restart trusted
state 覆盖补齐、snapshot/catch-up/leader 切换一致性回归、跨平台测试入口、
失败定位与 Windows/macOS follow-up 规划。

## Technical Context

**Language/Version**: C++20  
**Primary Dependencies**: gRPC, Protobuf, GoogleTest, CMake, standard library  
**Storage**: `meta.bin`, `log/segment_*.log`, snapshot catalog and snapshot directories under `NodeConfig::data_dir` and `SnapshotConfig::snapshot_dir`  
**Testing**: GoogleTest + CTest + Linux `test.sh`; planned platform-neutral `ctest --preset` entry and Windows PowerShell wrapper  
**Target Platform**: Linux as primary validation; Windows/macOS as supported design targets with explicit follow-up scope  
**Project Type**: Cross-platform Raft-based consistency layer / distributed storage metadata substrate  
**Performance Goals**: No new throughput target in this feature; preserve current steady-state behavior, avoid unbounded extra memory amplification on recovery/snapshot paths, and keep the low-parallel regression path runnable for local validation  
**Constraints**: Preserve protocol semantics, persisted formats, public API behavior, verified stable paths, and existing target names; avoid broad rewrites; prefer `std::filesystem`; no silent durability downgrade on any platform  
**Scale/Scope**: Incremental hardening of `storage`, `node`, `replication`, `state_machine`, `tests`, CMake/test entrypoints, and industrialization documentation

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

### Pre-Design Gate: PASS

- 已明确受保护基线：leader election、AppendEntries、majority commit、
  apply progression、snapshot save/load/install、follower catch-up baseline、
  segmented log persistence、restart recovery 主链路、`003` durability phases。
- 当前计划不包含协议语义、公共 API、持久化格式、命名或大规模架构重写。
- 所有高风险工作都显式绑定到 crash/restart/trusted-state 语义。
- Linux-specific 验证与 Windows/macOS fallback 已纳入范围定义。
- 测试入口将继续以 CTest 为统一执行面，并保留 Linux 脚本入口。

### Post-Design Gate: PASS

- 计划中的生产代码改动被限制为最小表面：优先 `modules/raft/storage`，
  只有在回归测试证明确有一致性问题时才扩大到 `node` / `replication` /
  `state_machine`。
- Linux-specific failure injection 被隔离为 test-only contract，不作为跨平台
  正确性的唯一证据。
- 计划要求每个高风险任务都带对应的失败诊断和验收标准。
- 没有必须破坏宪法的设计项；如后续确需新增窄范围 durability helper，
  也属于允许的新模块边界，不构成治理违规。

## Current Capability Treatment

| 分类 | 当前能力 | 当前状态 | 计划处理方式 |
|------|----------|----------|--------------|
| A 已完成且保留 | 选举、日志复制、majority commit、apply、snapshot save/load/install、基础 catch-up、restart recovery 主链路 | 稳定，已有测试和代码证据 | 不重新规划实现，只作为回归基线 |
| A 已完成且保留 | `003-persistence-reliability` 中已完成的 segment/meta/snapshot durability hardening 与 trusted-state diagnostics | 已完成，不能重复立项 | 仅复用成果，不重复实现 |
| B 已完成但补测试 | snapshot restart、split-brain、follower lag after compaction、apply/restart 边界 | 部分覆盖存在时序空洞 | 补回归和稳定性测试，不先改业务语义 |
| C 已实现但需修复 | Final Linux Validation 中暴露的 flaky 测试路径与不足的 failure localization | 影响验收可信度 | 优先修复测试稳定性和诊断，如证明确有代码缺陷再最小修复生产代码 |
| D 已实现但有跨平台风险 | `test.sh`、有限的 CMake presets、Windows 路径/flush/rename 运行时验证缺口 | Linux 主导，跨平台证据不足 | 增加平台无关入口、显式 fallback、后续适配任务 |
| E 半成品 | 精确 fsync/rename/prune/partial write failure injection、PowerShell 入口、统一失败矩阵 | 缺失或只做了分析 | 作为本 feature 的新增实施范围 |
| F 工业化新增 | 一键回归入口、失败日志定位、平台能力矩阵、Linux-specific 隔离说明、Windows/macOS follow-up 计划 | 当前不足 | 增加脚本、文档、contracts 和少量支持代码 |
| G 暂不处理 | 协议改造、transport 重写、snapshot 流式 RPC、业务存储引擎扩展 | 超出范围 | 显式 deferred，不进入当前 tasks |

## Project Structure

### Documentation (this feature)

```text
specs/004-raft-industrialization/
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── contracts/
│   ├── validation-entrypoints.md
│   └── failure-injection-boundaries.md
└── tasks.md
```

### Expected Source Impact

```text
modules/raft/storage/
├── raft_storage.cpp
├── snapshot_storage.cpp
└── [optional narrow durability helper if duplication blocks test-only injection]
modules/raft/node/
└── raft_node.cpp [only if recovery boundary or restart consistency bug is proven]
modules/raft/replication/
└── replicator.cpp [only if catch-up or leader-switch bug is proven]
modules/raft/state_machine/
└── state_machine.cpp [only if apply/recovery inconsistency is proven]
tests/
├── test_raft_segment_storage.cpp
├── test_snapshot_storage_reliability.cpp
├── test_raft_snapshot_restart.cpp
├── test_raft_snapshot_catchup.cpp
├── test_raft_snapshot_diagnosis.cpp
├── test_raft_split_brain.cpp
├── test_raft_replicator_behavior.cpp
├── raft_integration_test.cpp
└── [new failure-injection and platform-entry tests as needed]
tests/CMakeLists.txt
test.sh
CMakePresets.json
[planned] test.ps1
docs/ or specs/ for validation and platform guidance
```

**Structure Decision**: 保持当前仓库布局不变。优先在已有模块内增量完善；
只有当 storage/snapshot 的 test-only injection 或平台 durability 封装必须
复用时，才允许新增窄范围 helper。

## Research-Driven Design Decisions

- 研究结论见 [research.md](./research.md)。
- 数据与任务模型见 [data-model.md](./data-model.md)。
- 维护者入口契约见 [contracts/validation-entrypoints.md](./contracts/validation-entrypoints.md) 和 [contracts/failure-injection-boundaries.md](./contracts/failure-injection-boundaries.md)。
- 验证与落地顺序见 [quickstart.md](./quickstart.md)。

## Change Surface Summary

| 任务类型 | 范围 | 说明 |
|----------|------|------|
| 只改测试 | flaky 稳定化、restart trusted-state matrix、catch-up/leader-switch regression、Linux-specific failure cases | 优先补证据，不先动业务逻辑 |
| 改生产代码 | test-only failure injection seam、必要的 restart/catch-up 边界修复、最小诊断增强 | 只在测试证明存在真实风险时改动 |
| 新增脚本 | `test.sh` 增强、`ctest --preset` 入口规范、`test.ps1` | 建立 Linux 主入口和跨平台 fallback |
| 新增文档 | 失败定位、平台能力矩阵、Linux-specific 边界说明、回归执行说明 | 保障可验证和可维护 |

## Workstreams And Execution Order

### W0. Baseline Freeze And Capability Map

- **Priority**: P0
- **目标**: 把“保留 / 补测试 / 修复 / 跨平台风险 / 新增能力 / deferred”固定为
  tasks 输入，避免后续重复规划已完成内容。
- **变更类型**: 文档
- **生产代码**: 否
- **Linux-specific**: 否
- **Windows/macOS fallback**: 否
- **验收标准**:
  - 计划和后续 tasks 明确复用 `003` 已完成项，不再把它们当作新实现任务。
  - 每个后续任务都能映射到本计划中的能力分类和优先级。

### W1. Linux Validation Baseline Stabilization

- **Priority**: P0
- **目标**: 先稳定当前 Final Linux Validation 中的 flaky 验收路径，不放宽
  snapshot recovery 或 split-brain 断言。
- **变更类型**: 测试、脚本、诊断文档
- **生产代码**: 默认否；若证明存在真实一致性 bug，允许最小修复
- **Linux-specific**: 是，主验证环境
- **Windows/macOS fallback**: 以 `ctest --preset` 和文档化期望替代，不要求同阶段
  具备同等时序型 crash 证据
- **主要文件**: `tests/test_raft_snapshot_restart.cpp`,
  `tests/test_raft_split_brain.cpp`, `test.sh`, validation docs
- **验收标准**:
  - 当前已知两个 Linux flaky 场景都有明确归因和处置方案。
  - 重复执行目标测试时，不再因纯时序误判阻塞总验收，或问题被明确升级为真实代码缺陷。
  - 失败输出能直接指向 snapshot recovery、leader election 或 cluster timing 相关子系统。

### W2. Exact Failure Injection For Durability Boundaries

- **Priority**: P0
- **目标**: 补齐 `fsync`、directory sync、rename/replace、remove/prune、
  partial write 等精确 failure injection 能力，用于验证 trusted-state 和 crash
  window。
- **变更类型**: 生产代码、测试、文档
- **生产代码**: 是，允许新增 test-only seam 或窄范围 helper
- **Linux-specific**: 是，精确 crash/durability 注入以 Linux 为主
- **Windows/macOS fallback**: 文档化 weaker guarantee 与未执行注入测试的原因；
  保持默认关闭时行为一致
- **主要文件**: `modules/raft/storage/raft_storage.cpp`,
  `modules/raft/storage/snapshot_storage.cpp`,
  `[optional helper under modules/raft/storage/]`,
  new/updated storage recovery tests
- **验收标准**:
  - 关闭注入时，现有持久化路径语义和持久化格式完全不变。
  - 注入测试能够稳定命中目标失败点并验证 trusted-state 结果。
  - 每个失败点都有可读的诊断信息，包含操作、路径、平台范围和恢复预期。

### W3. Restart Trusted-State Matrix Completion

- **Priority**: P0
- **目标**: 系统化补齐 `current_term`、`voted_for`、`commit_index`、
  `last_applied`、log entries、snapshot metadata 的重启恢复组合覆盖。
- **变更类型**: 测试为主，必要时最小诊断增强
- **生产代码**: 可选，仅在真实恢复缺陷被证明时
- **Linux-specific**: 否，主体为平台无关恢复逻辑；部分 crash-like 场景在 Linux 标注
- **Windows/macOS fallback**: 同样执行逻辑类恢复测试；不宣称 crash 语义等价
- **主要文件**: `tests/persistence_test.cpp`,
  `tests/test_raft_segment_storage.cpp`,
  `tests/test_raft_snapshot_restart.cpp`,
  `tests/test_raft_snapshot_diagnosis.cpp`
- **验收标准**:
  - hard state、log boundary、snapshot selection、apply replay 的关键组合都有明确测试。
  - 非法 snapshot 或 boundary mismatch 不会被误接受为可信状态。
  - 恢复后的 commit/apply/snapshot/log 摘要对失败定位足够清晰。

### W4. Catch-Up, Snapshot Handoff, And Leader Switch Consistency

- **Priority**: P1
- **目标**: 强化 follower lag/restart、snapshot handoff、post-snapshot replay、
  leader 切换后一致性与 commit/apply 顺序回归覆盖。
- **变更类型**: 测试为主，必要时最小生产代码修复
- **生产代码**: 仅在测试揭示真实一致性缺陷时
- **Linux-specific**: 否
- **Windows/macOS fallback**: 逻辑回归测试同样适用；时序灵敏场景需标注未实机覆盖
- **主要文件**: `tests/test_raft_snapshot_catchup.cpp`,
  `tests/test_raft_replicator_behavior.cpp`,
  `tests/test_raft_log_replication.cpp`,
  `tests/test_raft_commit_apply.cpp`,
  `tests/raft_integration_test.cpp`,
  `modules/raft/replication/replicator.cpp`,
  `modules/raft/node/raft_node.cpp`
- **验收标准**:
  - follower 在落后 live log 和 snapshot boundary 两种情况下都能恢复到一致状态。
  - leader 切换后，已提交状态保持不变，新提交状态不会出现 commit/apply 逆序。
  - 若触发生产代码修改，必须有对应前置失败测试和修复后回归证据。

### W5. State Machine Apply And Restart Consistency Hardening

- **Priority**: P1
- **目标**: 验证并必要时修复状态机 apply、snapshot save/load 和 restart replay
  之间的一致性边界。
- **变更类型**: 测试、必要时最小生产代码、文档
- **生产代码**: 可选
- **Linux-specific**: 否
- **Windows/macOS fallback**: 逻辑类测试同样执行；durability 语义不外推
- **主要文件**: `modules/raft/state_machine/state_machine.cpp`,
  `modules/raft/node/raft_node.cpp`,
  `tests/test_state_machine.cpp`,
  `tests/test_raft_snapshot_restart.cpp`,
  `tests/raft_integration_test.cpp`
- **验收标准**:
  - restart 后状态机视图与可信的 committed+applied state 一致。
  - snapshot load 与后续 replay 不会重复 apply 或遗漏已提交状态。
  - 如果无需改生产代码，也必须留下覆盖该边界的测试证据。

### W6. Cross-Platform Validation Entry Consolidation

- **Priority**: P2
- **目标**: 把 Linux bash 入口、CTest preset、PowerShell fallback 和分组规则
  整理成统一维护者入口。
- **变更类型**: 脚本、CMake、测试组织、文档
- **生产代码**: 否
- **Linux-specific**: 部分，是对 `test.sh` 的保留和隔离说明
- **Windows/macOS fallback**: 是，新增 `ctest --preset` 和 `test.ps1` 入口规划
- **主要文件**: `test.sh`, `CMakePresets.json`, `tests/CMakeLists.txt`,
  `[planned] test.ps1`, validation docs
- **验收标准**:
  - Linux 有清晰的一键主入口，并保留低并发稳定运行方式。
  - Windows/macOS 至少具备不依赖 Bash 的构建/测试入口说明或脚本。
  - Linux-specific 测试组、平台无关测试组和 deferred 平台项界限清楚。

### W7. Failure Localization And Industrialization Documentation

- **Priority**: P3
- **目标**: 建立失败矩阵、日志保留说明、平台能力说明和回归执行文档。
- **变更类型**: 文档、脚本少量增强
- **生产代码**: 否
- **Linux-specific**: 部分，涉及 crash-like 结果留档
- **Windows/macOS fallback**: 是，以明确未覆盖项和后续计划为主
- **主要文件**: docs/spec artifacts, `test.sh` output conventions, quickstart
- **验收标准**:
  - 持久化、restart recovery、snapshot、cluster consistency 四类问题都有失败定位入口。
  - 文档能说明哪些测试保留数据、看哪些路径、哪些属于 Linux-specific。
  - 新增文档不重复解释已稳定的协议主路径，而是专注剩余工业化缺口。

### W8. Windows/macOS Deep Follow-Up And CI Expansion

- **Priority**: P4
- **目标**: 为 Windows/macOS 实机验证、CI 矩阵和更深入的 durability 适配预留
  独立 follow-up 范围。
- **变更类型**: 文档、CMake、脚本、CI 规划
- **生产代码**: 默认否；仅在平台差异暴露真实缺口时进入后续 feature
- **Linux-specific**: 否
- **Windows/macOS fallback**: 本任务本身就是 fallback 与适配定义
- **主要文件**: future CI docs, presets, platform notes
- **验收标准**:
  - 明确列出 Windows/macOS 当前未验证项、禁止宣称等价的区域和后续执行顺序。
  - 若新增 preset 或脚本，必须不破坏现有 Linux 主路径。

## Execution Dependencies

1. `W0` 必须先完成，冻结范围和分类基线。
2. `W1` 必须早于 `W2`-`W5`，否则后续回归仍会被现有 flaky 噪声污染。
3. `W2` 与 `W3` 可在 `W1` 后并行设计，但 `W2` 完成后才能声称覆盖精确 crash window。
4. `W4` 与 `W5` 在 `W3` 完成 trusted-state 基线后执行，避免把恢复问题和复制问题混淆。
5. `W6` 汇总前面工作形成统一入口，不应早于核心回归范围稳定。
6. `W7` 在 `W1`-`W6` 的输出基础上形成最终诊断与执行文档。
7. `W8` 作为当前 feature 的尾部 follow-up，不阻塞 Linux 主验证收敛。

## Validation Strategy

- **Linux primary**:
  - `cmake --preset debug-ninja-low-parallel`
  - `cmake --build --preset debug-ninja-low-parallel`
  - `CTEST_PARALLEL_LEVEL=1 ./test.sh --group all`
  - 目标子集按 `unit / persistence / snapshot-recovery / diagnosis / snapshot-catchup / replicator / segment-cluster` 拆分复验
- **Platform-neutral fallback**:
  - `ctest --preset debug-tests --output-on-failure`
  - PowerShell 入口或文档化等价命令用于 Windows/macOS
- **Linux-specific validation**:
  - 精确 failure injection、power-loss approximation、可能依赖进程/信号/目录 fsync 语义的测试单独分组、单独标注
- **Failure retention**:
  - 继续使用 `RAFT_TEST_KEEP_DATA=1` / `--keep-data` 进行现场保留
  - 每个高风险测试分组都要能映射到 storage / snapshot / node / replication 中的具体诊断面

## Acceptance By Priority

- **P0**:
  - Linux 主验收不再被已知 timing-only flaky 无差别阻塞。
  - trusted-state recovery 与精确 durability failure path 有稳定证据。
- **P1**:
  - catch-up、snapshot handoff、leader switch、apply/restart 一致性边界有回归测试和必要修复。
- **P2**:
  - Bash 之外存在明确的跨平台构建/测试入口。
- **P3**:
  - 失败定位、平台范围和执行说明文档齐全。
- **P4**:
  - Windows/macOS 深度验证与 CI 扩展被明确排程，而不是隐式遗留。

## Complexity Tracking

当前没有宪法违规项需要豁免。若后续 tasks 确认必须引入新的窄范围
durability helper 或 Linux-specific failure harness，将以“最小新模块、默认关闭、
不改变默认生产行为”为前提继续执行，不视为大规模重构。
