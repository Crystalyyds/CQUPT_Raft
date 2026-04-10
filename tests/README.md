# tests 目录说明

这个目录用于放项目的单元测试与阶段性集成测试。

## 当前测试文件

### 基础模块测试
- `test_command.cpp`
- `test_state_machine.cpp`
- `test_min_heap_timer.cpp`
- `test_thread_pool.cpp`

### Raft 流程测试
- `test_raft_election.cpp`
- `test_raft_log_replication.cpp`
- `test_raft_commit_apply.cpp`

## 说明

- 基础模块测试主要验证单个组件的输入输出行为。
- Raft 流程测试会实际启动 3 个节点，属于轻量级集成测试。
- 这几类测试依赖时间窗口、线程调度和本地端口，因此比纯逻辑测试更容易受环境影响。
- 目前这些 Raft 流程测试主要覆盖正常路径，还没有覆盖 leader 宕机、网络分区、日志冲突恢复等故障场景。
