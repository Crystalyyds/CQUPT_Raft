# Contract: 验证入口约定

## 目标

定义维护者执行工业化验证时应使用的入口约定，并与
[../validation-matrix.md](../validation-matrix.md) 保持一致。该约定只说明
验证入口、解释规则和平台边界，不改变 CQUPT_Raft 的协议语义、持久化格式或
公共 API。

## 入口分类

### 1. Linux 主验证入口

- **当前基线**：
  - `cmake --preset debug-ninja-low-parallel`
  - `cmake --build --preset debug-ninja-low-parallel`
  - `CTEST_PARALLEL_LEVEL=1 ./test.sh --group all`
- **约定**：
  - 这是当前唯一的 Linux 主验收路径。
  - 它与 [../validation-matrix.md](../validation-matrix.md) 中的
    `Linux primary` 定义一致。
  - 它可以承载 Linux-specific 分组、低并发回归和 `--keep-data` 现场保留。
  - 它不能被当作非 Linux 平台唯一入口。

### 2. 平台无关 CTest fallback

- **当前基线**：
  - `ctest --preset debug-tests --output-on-failure`
- **约定**：
  - 这是当前跨平台 baseline fallback。
  - 它应覆盖平台无关的回归子集。
  - 它不能自动替代 Linux-specific crash-style、failure-injection 或
    `--keep-data` 证据。
  - 文档必须明确说明：当某项在
    [../validation-matrix.md](../validation-matrix.md) 中被标注为
    Linux-specific 时，该入口只提供逻辑回归 fallback。

### 3. Windows/macOS fallback 或 deferred 入口

- **当前状态**：
  - 计划中的 PowerShell wrapper 或等价非 Bash 命令序列，尚未落地
- **约定**：
  - 在正式的 `test.ps1` 或等价脚本出现前，Windows/macOS 当前只能依赖
    `ctest --preset debug-tests --output-on-failure` 作为 fallback。
  - 任何依赖 Bash、`--keep-data`、Linux-specific failure injection 或
    crash-style 语义的流程，都必须在 Windows/macOS 上标注为 deferred，
    而不是隐式视为已完成。

## 验证顺序约定

推荐顺序必须与 [../validation-matrix.md](../validation-matrix.md) 和
`quickstart.md` 保持一致：

1. 先查看 `validation-matrix.md`，确认当前能力分类、Linux-specific 边界和
   deferred 项。
2. 在 Linux 上使用 `cmake --preset debug-ninja-low-parallel` 与
   `cmake --build --preset debug-ninja-low-parallel` 完成低并发构建。
3. 使用 `CTEST_PARALLEL_LEVEL=1 ./test.sh --group all` 作为 Linux 主回归入口。
4. 如果出现高风险失败，按分组 rerun 命令进入 focused regression。
5. 如果当前环境不走 Bash 主入口，使用
   `ctest --preset debug-tests --output-on-failure` 作为平台无关 fallback。

## 真实分组与 rerun 约定

以下分组直接来自 `test.sh`，文档和任务必须使用这些真实名称：

- `persistence`
- `snapshot-recovery`
- `diagnosis`
- `snapshot-catchup`
- `replicator`
- `replication`
- `election`
- `segment-cluster`

推荐解释：

- `persistence`：重启恢复、hard state、log trusted-state 边界
- `snapshot-recovery`：snapshot/restart 主恢复路径
- `diagnosis`：恢复诊断、skip/fallback、边界错误定位
- `snapshot-catchup`：lagging follower 追赶与 snapshot handoff
- `replicator`：单 follower 复制状态机行为
- `replication`：日志复制与 commit/apply 主路径
- `election`：选举与 split-brain 路径
- `segment-cluster`：高负载 segment/snapshot 组合路径

## Linux-specific 分组与解释规则

下列证据必须显式按 Linux-primary 或 Linux-specific 解释：

- 时序敏感的 flaky 验收路径
- 未来的 exact durability failure injection 分组
- 依赖 Bash-first 执行和 `--keep-data` 现场保留的流程

因此：

- 文档中出现这些分组时，必须说明它们不是跨平台 runtime 语义已完成的证明。
- Windows/macOS 当前只能把它们视为 follow-up 或 deferred 项。

## 失败现场保留约定

- Linux 主入口必须支持：
  - `CTEST_PARALLEL_LEVEL=1 ./test.sh --group all --keep-data`
  - `./test.sh --group persistence --keep-data`
  - `./test.sh --group snapshot-recovery --keep-data`
  - `./test.sh --group diagnosis --keep-data`
- 文档必须明确：
  - `--keep-data` 是 Linux Bash 主入口能力
  - `ctest --preset debug-tests` 不默认提供等价现场保留能力
  - 若需要 retained artifacts 做排障，应回到 Linux 主入口重跑

## 非目标

- 不新增任何公共验证 API
- 不修改协议或持久化格式
- 不宣称 Windows/macOS 已具备与 Linux 等价的 crash-style runtime 证据
