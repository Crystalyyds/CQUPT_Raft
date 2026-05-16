# Platform Support Matrix

## 目标

记录 `004-raft-industrialization` 当前在 Linux / Windows 的平台支持状态、
验证入口、已验证范围、未验证范围和后续 follow-up，避免把
platform-neutral baseline、cluster/runtime-heavy、durability-boundary、
linux-specific-failure-injection 混为一谈。

本文件只记录当前证据和解释边界，不改变 CQUPT_Raft 的协议语义、持久化格式、
公共 API 或测试行为。

## 范围说明

- Linux：当前主验证平台
- Windows：当前提供保守 fallback 验证平台
- macOS：当前不在本 feature 验证范围内

## 术语边界

### platform-neutral baseline

- 可用于跨平台逻辑回归的保守基线
- 当前 Windows 默认只声明这一层的保守子集，不代表 Raft 全功能都已验证

### cluster/runtime-heavy

- 需要更长链路运行时行为、集群协同、时序稳定性或更高操作负载的回归
- 当前主要仍以 Linux 主环境解释

### durability-boundary

- restart / trusted-state / crash-style / durability boundary 语义
- 这类能力即使可以通过 CTest 运行，也不自动等价为跨平台运行时证据

### linux-specific-failure-injection

- exact `fsync`、directory sync、replace/rename、prune/remove、partial write
  等精确 failure-injection 语义
- 当前只记录 Linux 主环境证据

## 平台支持矩阵

| 平台 | 当前角色 | 主验证入口 | 当前状态 | 已验证范围 | 未验证范围 / 不声明事项 |
|------|----------|------------|----------|------------|-------------------------|
| Linux | 主验证平台 | `./test.sh` | 主入口有效；低并发 configure/build 有效；`debug-tests` 仍有现存运行期红灯 | Linux Bash 主入口、低并发构建、`persistence` 主回归、Linux-specific durability / failure-injection / crash-style 解释边界 | 不声明 `ctest --preset debug-tests` 已全绿；不把 cluster/runtime-heavy 现存红灯解释成标签或入口语义错误 |
| Windows | 保守 fallback 平台 | `.\test.ps1 -All` | preset configure/build/test fallback 已记录为 PASS | Visual Studio 2022 MSVC x64 下的保守 platform-neutral baseline 子集：`CommandTest`、`KvStateMachineTest`、`TimerSchedulerTest`、`ThreadPoolTest` | 不声明 Windows Raft 全功能测试通过；不声明 Windows 已等价验证 Linux-specific durability / failure-injection；不声明 cluster-style 测试已完成稳定性验证 |
| macOS | 不在本 feature 验证范围内 | N/A | 当前不在本 feature 验证范围内 | 无 | 不写已验证，不声明任何等价运行时证据 |

## 各平台验证入口

### Linux

- configure preset：`debug-ninja-low-parallel`
- build preset：`debug-ninja-low-parallel`
- CTest fallback：`debug-tests`
- Bash 主入口：`./test.sh`

当前记录的结果：

- `cmake --preset debug-ninja-low-parallel`：PASS
- `cmake --build --preset debug-ninja-low-parallel`：PASS
- `./test.sh --group persistence`：PASS
- `ctest --preset debug-tests --output-on-failure`：FAIL

解释规则：

- Linux 是当前唯一主验收平台。
- Linux-specific durability / failure-injection / crash-style 语义继续以
  `./test.sh` 主入口解释。
- `ctest --preset debug-tests --output-on-failure` 在 Linux 上仍是 CTest
  fallback，但它不替代 Linux-primary 主验收语义。
- 当前 `debug-tests` 的 FAIL 应解释为 cluster/runtime-heavy 现存红灯，
  不是 `tests/CMakeLists.txt` 标签契约失效，也不是“Linux 主入口失效”。

### Windows

- generator / toolchain：Visual Studio 17 2022 / MSVC x64
- configure preset：`windows`
- build preset：`windows-release`
- test preset：`windows-release-tests`
- PowerShell fallback：`test.ps1`

当前记录的结果：

- `cmake --preset windows`：PASS
- `cmake --build --preset windows-release`：PASS
- `ctest --preset windows-release-tests`：PASS

解释规则：

- Windows 当前只提供保守的 platform-neutral fallback。
- `test.ps1` 只是对 `windows` / `windows-release` /
  `windows-release-tests` 的一键封装。
- Windows 默认覆盖的 test-name 子集是：
  `CommandTest`、`KvStateMachineTest`、`TimerSchedulerTest`、
  `ThreadPoolTest`。
- 这不代表所有带有部分 `platform-neutral` 逻辑的受管测试都已在 Windows
  进入默认验收范围。

## 验证范围分层

| 分类 | Linux 当前记录 | Windows 当前记录 | macOS 当前记录 |
|------|----------------|------------------|----------------|
| platform-neutral baseline | 已记录为可用；Linux 可通过 `./test.sh` 与 `ctest --preset debug-tests` 承载逻辑回归 | 只记录保守 baseline 子集已通过 | 当前不在本 feature 验证范围内 |
| cluster/runtime-heavy | 当前仍有现存红灯，主要在 `debug-tests` 全量运行中暴露；主解释继续归 Linux | 不声明已覆盖，也不声明已通过 | 当前不在本 feature 验证范围内 |
| durability-boundary | 已记录 Linux 主环境解释与相关恢复边界 | 不声明等价运行时验证 | 当前不在本 feature 验证范围内 |
| linux-specific-failure-injection | 当前只记录 Linux 证据 | 明确不等价、不继承 | 当前不在本 feature 验证范围内 |

## Linux-specific 任务交叉检查

下表用于把 `T006/T007/T011/T023-T030` 中涉及 Linux-specific failure
injection、crash-style、Bash-first 与 durability-boundary 的内容，同当前平台
支持边界逐项对齐。

| 任务 / 内容 | Linux 当前解释 | Windows 当前 fallback | 未等价验证 / deferred 说明 | macOS |
|------------|----------------|-----------------------|----------------------------|-------|
| T006 / T009：meta / log failure injection | exact `fsync`、directory sync、replace/rename、partial write、trusted-state 边界只记为 Linux-specific runtime evidence | 只允许回退到 `ctest --preset debug-tests` 的 logic fallback，或 Windows preset / `test.ps1` 的保守 platform-neutral baseline | 不声明 Windows 已完成等价 durability 运行时验证；后续继续保留 Windows durability 语义适配与 runtime validation follow-up | 当前不在本 feature 验证范围内 |
| T007 / T010：snapshot publish / prune failure injection | temp-dir publish、replace/rename、prune/remove、snapshot durability boundary 继续由 Linux 主验收解释 | 只允许回退到 Linux `debug-tests` logic fallback，或 Windows preset / `test.ps1` 的保守 baseline；不把 publish/prune 注入语义转写为 Windows 已验证 | Windows snapshot publish/prune 等价语义仍属 deferred / follow-up，不记录为已验证 | 当前不在本 feature 验证范围内 |
| T011：injected-failure diagnostics | retained-artifact、diagnosis、crash-style 失败定位继续按 Linux-primary 解释 | Windows 只保留 platform-neutral fallback，不提供与 `--keep-data` 或 Linux-primary diagnosis 等价的排障能力 | Windows retained-artifact / diagnosis / crash-style 运行时观察继续保留为 follow-up | 当前不在本 feature 验证范围内 |
| T023：Linux Bash 主入口 | `./test.sh`、低并发分组和 `--keep-data` 是 Linux 主验收路径 | Windows fallback 入口是 preset + `.\test.ps1 -All`；Linux 的 `ctest --preset debug-tests` 只是 CTest fallback | 不把 Windows preset / PowerShell fallback 写成 Bash-first 等价入口 | 当前不在本 feature 验证范围内 |
| T024 / T025：Windows preset / PowerShell fallback | Linux 主入口语义不变 | `cmake --preset windows`、`cmake --build --preset windows-release`、`ctest --preset windows-release-tests`、`.\test.ps1 -All` 只代表保守 platform-neutral baseline | 不声明 Windows Raft 全功能通过；cluster-style、durability、failure-injection 继续 deferred | 当前不在本 feature 验证范围内 |
| T026：CTest label / Linux-specific 边界 | `platform-neutral`、`durability-boundary`、`linux-specific-failure-injection`、`linux-primary-diagnosis` 当前都按 Linux 主环境边界解释 | Windows fallback 只消费保守 baseline 子集，不因为 label 存在就继承 Linux-specific 语义 | mixed executable 不会被写成 Windows 已等价验证；Linux-specific 语义仍保留为 deferred runtime follow-up | 当前不在本 feature 验证范围内 |
| T028 / T030：manual-only / diagnostic-only / temporary 文件 | `persistence_more_test.cpp`、`test_temp.cpp` 不属于 Linux 主回归入口，只作为手工诊断或临时文件说明 | 不进入 Windows platform-neutral fallback，也不作为跨平台正式验收资产 | 若后续清理或迁移，仅作为 follow-up 记录，不在当前范围执行 | 当前不在本 feature 验证范围内 |

## 已验证范围

### Linux 已验证范围

- `debug-ninja-low-parallel` configure/build 入口可用
- `./test.sh` 作为 Linux 主入口仍可用
- `./test.sh --group persistence` 仍可用
- `tests/CMakeLists.txt` 中 `platform-neutral`、`durability-boundary`、
  `linux-specific-failure-injection`、`linux-primary-diagnosis` 的标签语义
  与文档解释保持一致
- Linux-specific durability / failure-injection / crash-style 语义仍由 Linux
  主环境负责解释

### Windows 已验证范围

- Visual Studio 2022 MSVC x64 下的 preset configure/build/test fallback 可用
- `test.ps1` 可作为 Windows PowerShell fallback 入口
- 当前默认只覆盖保守 platform-neutral baseline 子集：
  `CommandTest`、`KvStateMachineTest`、`TimerSchedulerTest`、`ThreadPoolTest`

## 未验证范围

### Linux 未验证或当前仍有红灯的范围

- `ctest --preset debug-tests --output-on-failure` 当前未达到全绿
- 现存失败应归类为 cluster/runtime-heavy 运行期红灯
- 因此当前不能把 Linux 的全量 CTest 结果写成“平台无关 baseline 全部已通过”

### Windows 未验证范围

- Windows cluster-style 测试扩大范围
- Windows 下的 Raft election / replication / snapshot / persistence
  稳定性验证
- Windows 等价 durability-boundary 运行时验证
- Windows 等价 linux-specific-failure-injection 语义验证
- Windows 下 Bash-first retained-artifact / `--keep-data` 等价流程

## 路径、临时目录与进程模型说明

### Linux

- 当前主验证流程基于 Bash：`./test.sh`
- retained-artifact 与 `--keep-data` 诊断流程只在 Linux 主入口解释
- durability 相关的 flush/sync、rename/replace、temp-dir publish/prune、
  crash-style 语义继续按 Linux 主环境记录

### Windows

- 当前文档化入口基于 Visual Studio preset 与 PowerShell wrapper
- 路径、临时目录、rename/replace、flush/sync 与 crash-style 语义目前不宣称
  已具备 Linux 等价证据
- 当前只把 Windows 记为 platform-neutral fallback，不记为 durability
  主验收平台

## 后续 Follow-Up

- 扩大 Windows cluster-style 测试覆盖范围，而不是停留在保守 baseline 子集
- 补齐 Windows 下的 Raft election / replication / snapshot / persistence
  稳定性验证
- 评估并记录 Windows durability 语义适配，包括 flush/sync、rename/replace、
  temp-dir publish/prune 与 crash-style 边界
- 扩展 CI matrix，使 Linux 主验证与 Windows fallback 的边界和结果可持续复现
- 若未来引入新的非 Bash 平台入口，必须继续保持“不虚报 Linux-specific
  等价证据”的文档约束

## 结论

- Linux 仍是当前主验证平台。
- Windows 仍是当前保守 fallback 平台。
- Windows fallback 不等价于 Linux-specific durability / failure-injection /
  crash-style 验收。
- 当前 `US3` 的入口文档和状态说明已经收口；剩余 cluster-style、
  durability 语义扩大验证和 CI 扩展继续保留为 follow-up。
- 当前不记录 macOS 已验证；macOS 仍不在本 feature 验证范围内。
