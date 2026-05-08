# CQUPT_Raft

一句话说明：这是一个基于 C++20、gRPC、Protobuf、GoogleTest 的 Raft KV 内核项目，重点在一致性内核、持久化、快照、追赶和重启恢复。

## 全局规则

- 平台相关的 durability 代码不允许静默降级。
- 如果一个平台实现了真实持久化语义，其他平台分支必须提供等价行为、返回明确错误，或在 durability contract 中明确记录较弱保证。
- 对 required durability operations，不允许使用 no-op 后直接返回成功的实现。

## 使用规则

- 先读根 `AGENTS.md`。
- 再根据模块索引进入目标模块目录的 `AGENTS.md`。
- 再读取该模块源码与相关测试。
- 不要默认扫描整个仓库。

## 模块索引

- `modules/raft/common`
  - 共享配置、提案结果、命令编解码
- `modules/raft/runtime`
  - 日志、定时器、线程池
- `modules/raft/service`
  - gRPC Raft/KV 服务适配层
- `modules/raft/node`
  - `RaftNode` 核心状态与调度
- `modules/raft/replication`
  - 单 follower 复制状态机
- `modules/raft/storage`
  - Raft 硬状态、segment log、snapshot catalog 持久化
- `modules/raft/state_machine`
  - KV 状态机与状态机快照格式
- `proto`
  - RPC/Protobuf 契约层
- `apps`
  - 节点入口与 CLI 客户端入口

## 全局硬规则

- 禁止读取 `.gitignore` 文件下面的文件夹里的任何文件。
- 禁止读取：
  - `vcpkg-configuration.json`
  - `CQUPT_Raft_AI_Context.md`
  - `README.md`
  - `/deploy`
- 不允许修改业务逻辑。
- 不允许修改协议语义。
- 不允许修改持久化格式。
- 不允许修改公共 API 行为。
- 不允许修改类名、函数名、命名空间。
- 不允许删除测试。
- 不允许跳过失败测试。
- 不允许为了通过编译而顺手改业务行为。

## 构建命令

```bash
cmake --preset debug-ninja-low-parallel
cmake --build --preset debug-ninja-low-parallel
```

更保守：

```bash
cmake --preset debug-ninja-safe
cmake --build --preset debug-ninja-safe
```

## 测试命令

```bash
./test.sh
./test.sh --group unit
./test.sh --group persistence
./test.sh --group all
./test.sh --keep-data
```

建议保持低并发，例如：

```bash
CTEST_PARALLEL_LEVEL=1 ./test.sh --group all
```

## Include 规则

- 项目源码和测试统一从 `modules/` 作为 include 根目录。
- 头文件按模块引用：
  - `raft/common/...`
  - `raft/runtime/...`
  - `raft/service/...`
  - `raft/node/...`
  - `raft/replication/...`
  - `raft/storage/...`
  - `raft/state_machine/...`
- 生成的 protobuf/gRPC 头文件保持直接引用：
  - `raft.pb.h`
  - `raft.grpc.pb.h`

## CMake 修改规则

- 优先保持现有 target 名称不变：
  - `raft_proto`
  - `raft_core`
  - `raft_demo`
  - `raft_kv_client`
- 只改路径、源文件列表、include 根目录和必要的构建脚本路径。
- 不要因为目录调整而引入新的业务逻辑分支。

## 跨模块修改规则

- 先定位主模块，再最小化波及范围。
- 如果修改 `modules/raft/node`，通常要检查：
  - `modules/raft/replication`
  - `modules/raft/storage`
  - `modules/raft/state_machine`
  - 相关 Raft 测试
- 如果修改 `proto` 或 `service`，必须同步检查契约使用方和测试。

## 高风险区域

- `proto/`
- `modules/raft/service`
- `modules/raft/storage`
- `modules/raft/node`
- `modules/raft/replication`
- snapshot/restart/catch-up 相关测试
- 并发与 crash recovery 路径

## 上下文节省规则

- 不要默认扫描整个仓库。
- 不要扫描NOTREAD.md文件里面标记的文件
- 先读根 `AGENTS.md`，再读目标模块 `AGENTS.md`。
- `.gitignore` 覆盖内容、构建产物、缓存、运行数据、日志、临时文件默认不读。
- 运行测试产生的 `raft_data/`、`raft_snapshots/`、`build/tests/raft_test_data/` 不要当作源码分析。
- 优先读：
  - `project_understanding.md`
  - `refactor_plan.md`
  - 根 `CMakeLists.txt`
  - `tests/CMakeLists.txt`
  - `proto/raft.proto`
  - 目标模块目录
  - 与目标模块直接相关的测试文件
