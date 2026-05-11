# Quickstart: CQUPT_Raft 工业化跨平台完善

## 1. 先读范围边界

- 功能范围与非目标：[spec.md](./spec.md)
- 执行顺序与验收标准：[plan.md](./plan.md)
- 风险冻结与验证矩阵：[validation-matrix.md](./validation-matrix.md)

`validation-matrix.md` 是本 feature 的单一验证基线。后续执行、rerun、平台解释和
Linux-specific 边界，都以它为准。

## 2. 当前推荐的验证顺序

1. 先阅读 [validation-matrix.md](./validation-matrix.md)，确认哪些能力已完成，
   哪些属于后续补强，哪些是 Linux-specific，哪些仍是 Windows/macOS deferred。
2. 使用 Linux 主入口完成低并发构建和主回归。
3. 若某个高风险区域失败，按矩阵中的 focused rerun 命令进入对应分组。
4. 若不在 Linux 上，或只需要跑平台无关基线，使用 `ctest --preset debug-tests`
   作为 fallback。

## 3. Linux 主验证入口

先构建：

```bash
cmake --preset debug-ninja-low-parallel
cmake --build --preset debug-ninja-low-parallel
```

再执行 Linux 主回归入口：

```bash
CTEST_PARALLEL_LEVEL=1 ./test.sh --group all
```

这是当前 Linux 主验证路径，与
[validation-matrix.md](./validation-matrix.md) 中的 `Linux primary` 定义保持一致。

## 4. 失败现场保留方式

当需要保留失败现场、测试数据目录或辅助排障时，使用：

```bash
CTEST_PARALLEL_LEVEL=1 ./test.sh --group all --keep-data
```

高风险分组的 rerun 也可以保留数据，例如：

```bash
./test.sh --group snapshot-recovery --keep-data
./test.sh --group diagnosis --keep-data
./test.sh --group persistence --keep-data
```

解释规则：

- `--keep-data` 只属于 Linux Bash 主入口能力。
- 平台无关 `ctest --preset debug-tests` fallback 不默认提供等价的数据保留语义。
- 如果需要 retained artifacts 进行排障，应优先回到 Linux 主入口重跑。

## 5. 高风险区域的真实 rerun 命令

这些命令来自当前 `test.sh` 分组，必须与
[validation-matrix.md](./validation-matrix.md) 保持一致：

```bash
./test.sh --group persistence
./test.sh --group snapshot-recovery
./test.sh --group diagnosis
./test.sh --group snapshot-catchup
./test.sh --group replicator
./test.sh --group replication
./test.sh --group election
```

推荐的解释方式：

- `persistence`：重启恢复、hard state、log trusted-state 边界
- `snapshot-recovery`：snapshot/restart 路径，当前也是 Linux flaky 风险热点
- `diagnosis`：恢复诊断、snapshot skip/fallback、边界错误定位
- `snapshot-catchup`：follower 落后后通过 log replay 或 snapshot handoff 追赶
- `replicator`：单 follower 复制状态机与 catch-up 行为
- `replication`：日志复制与 commit/apply 主路径回归
- `election`：选举与 split-brain 相关回归

## 6. 平台无关 CTest fallback

如果当前环境不走 Bash 主入口，或只需要执行平台无关的基础回归，使用：

```bash
ctest --preset debug-tests --output-on-failure
```

解释规则：

- 这是当前唯一明确的跨平台 baseline fallback。
- 它适合平台无关的逻辑回归，不自动等价于 Linux-specific crash-style 或 failure-injection 证据。
- 当 `validation-matrix.md` 中某项被标记为 Linux-specific 时，`ctest --preset debug-tests`
  只能作为逻辑回归 fallback，不能替代 Linux runtime 语义验证。

## 7. Linux-specific 测试组说明

以下内容必须视为 Linux-primary 或 Linux-specific 证据，而不是跨平台已完成能力：

- 当前时序敏感的 flaky 验收路径
- 未来的精确 `fsync`、directory sync、rename/replace、prune/remove、
  partial write failure injection
- 任何依赖 Bash-first 测试编排和 `--keep-data` 现场保留的流程

因此：

- 文档、计划和任务中提到的 Linux-specific 分组，必须显式标注。
- 这些分组在 Windows/macOS 上不能被默认为“等价完成”。

## 8. Windows/macOS fallback 与 deferred 说明

当前阶段对 Windows/macOS 的要求是：

- 至少能理解并使用平台无关 fallback：

```bash
ctest --preset debug-tests --output-on-failure
```

- 需要明确知道，Linux-specific durability / failure-injection / crash-style
  证据尚未在 Windows/macOS 上完成 runtime 验证。
- 若后续需要更明确的非 Bash 入口，应由后续任务补充 `test.ps1` 或等价说明；
  当前阶段仍属于 planned fallback / deferred scope。

## 9. 当前实现顺序

1. 冻结已完成能力和剩余风险边界
2. 收敛 Linux flaky 验证路径
3. 增加 exact durability failure injection
4. 补齐 restart trusted-state 覆盖
5. 强化 catch-up、leader switch、apply/replay consistency
6. 收敛 CTest、Bash 和后续 PowerShell 入口
7. 完成失败定位与平台支持文档
