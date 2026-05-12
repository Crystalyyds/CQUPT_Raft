# Phase 4A: Snapshot Atomic Publish Analysis And Plan

## 1. 读取了哪些文件

- 根 `AGENTS.md`
- `modules/raft/storage/AGENTS.md`
- `modules/raft/node/AGENTS.md`
- `modules/raft/state_machine/AGENTS.md`
- `docs/PERSISTENCE_DURABILITY_CONTRACT.md`
- `specs/003-persistence-reliability/spec.md`
- `specs/003-persistence-reliability/plan.md`
- `specs/003-persistence-reliability/tasks.md`
- `specs/003-persistence-reliability/progress.md`
- `specs/003-persistence-reliability/decisions.md`
- `modules/raft/storage/snapshot_storage.h`
- `modules/raft/storage/snapshot_storage.cpp`
- `modules/raft/state_machine/state_machine.h`
- `modules/raft/state_machine/state_machine.cpp`
- `modules/raft/node/raft_node.cpp` 中 startup recovery、`OnInstallSnapshot`、`LoadLatestSnapshotOnStartup`、`SnapshotWorkerLoop`、`CompactLogPrefixLocked` 相关路径
- `tests/test_snapshot_storage_reliability.cpp`
- `tests/snapshot_test.cpp`
- `tests/test_raft_snapshot_restart.cpp`
- `tests/test_raft_snapshot_diagnosis.cpp`

## 2. 当前 snapshot 路径分析

### 2.1 本地 snapshot save / publish

- `RaftNode::SnapshotWorkerLoop`
- `KvStateMachine::SaveSnapshot(snapshot_work_node_<id>.bin)`
- `FileSnapshotStorage::SaveSnapshotFile(temp_snapshot_file, last_applied, term, ...)`
- 成功后 `CompactLogPrefixLocked(...)`
- 然后 `PersistStateLocked(...)`
- 最后 `PruneSnapshots(...)`

当前 `KvStateMachine::SaveSnapshot()` 只负责生成状态机快照工作文件。当前 `FileSnapshotStorage::SaveSnapshotFile()` 才负责把工作文件复制为 Raft 可恢复的 snapshot 目录对象。

### 2.2 follower install snapshot publish

- `RaftNode::OnInstallSnapshot`
- 写 `install_snapshot_node_<id>.bin.tmp`
- `FileSnapshotStorage::SaveSnapshotFile(...)`
- `state_machine_->LoadSnapshot(saved_meta.snapshot_path)`
- `CompactLogPrefixLocked(...)`
- `PersistStateLocked(...)`
- `PruneSnapshots(...)`

### 2.3 startup snapshot load / recovery

- `storage_->Load(...)` 先恢复 `meta.bin + log`
- `RaftNode::LoadLatestSnapshotOnStartup()`
- `snapshot_storage_->ListSnapshots(...)`
- 对排序后的 snapshot 逐个执行 `state_machine_->LoadSnapshot(meta.snapshot_path)`
- 成功后执行 `CompactLogPrefixLocked(...)`
- 更新 `commit_index_` / `last_applied_`
- `PersistStateLocked(...)`
- 构造函数后续再执行 `ApplyCommittedEntries()`

### 2.4 snapshot data 和 metadata 的关系

- `snapshot_<index>/data.bin` 保存状态机快照内容
- `snapshot_<index>/__raft_snapshot_meta` 保存 `last_included_index`、`last_included_term`、`created_unix_ms`、数据文件名和 checksum
- trusted snapshot 必须同时满足：
  - meta header / version 正确
  - snapshot 目录名与 meta 中的 index 一致
  - `data.bin` 存在
  - checksum 匹配

## 3. 当前 durability 缺口

### 3.1 直接写最终目录

当前 `FileSnapshotStorage::SaveSnapshotFile()`：

- 直接定位最终 `snapshot_<index>/`
- 如果已存在则先 `remove_all(final_dir)`
- `create_directories(final_dir)`
- 直接在最终目录内写 `data.bin`
- 再直接在最终目录内写 `__raft_snapshot_meta`

这意味着当前没有 temp snapshot dir，也没有单独的原子 publish 点。

### 3.2 file fsync 缺口

- `KvStateMachine::SaveSnapshot()` 的工作文件写入只做 `flush`
- `CopyFilePortable()` 写 `data.bin` 后没有 file `fsync`
- snapshot metadata 写入后没有 file `fsync`
- install-snapshot 临时文件写入后没有 file `fsync`

### 3.3 directory fsync 缺口

- `snapshot_dir` 创建后没有 directory `fsync`
- `snapshot_<index>/` 创建后没有 directory `fsync`
- snapshot publish 后没有对 `snapshot_dir` 父目录做 directory `fsync`
- prune 删除旧 snapshot 后没有 directory `fsync`
- state machine 工作文件 `rename` 后也没有父目录 durability barrier

## 4. 当前 crash window

- `remove_all(final_dir)` 之后到新目录完全写好之前：旧 snapshot 已删，新 snapshot 未完成
- `data.bin` 已写但 `__raft_snapshot_meta` 未写
- meta 已写但目录项未 durable
- snapshot 已成功发布，但 `CompactLogPrefixLocked()` / `PersistStateLocked()` 尚未完成
- install-snapshot 路径中 snapshot 已发布，但 `LoadSnapshot()` 或 raft state persist 可能随后失败
- `PruneSnapshots()` 删除旧 snapshot 后如果崩溃，没有 durable 删除边界

## 5. 当前 trusted-state 判定

- `snapshot_storage_->ListSnapshots()` 只返回通过目录名、meta、数据文件和 checksum 校验的 snapshot
- 排序规则是 `last_included_index` 降序，其次 `created_unix_ms` 降序
- restart recovery 采用“最新有效优先；无效跳过；允许回退到更旧有效 snapshot”
- 当前 trusted-state 判定已经允许恢复逻辑容忍“新 snapshot 无效、回退旧 snapshot”的路径

## 6. 需要修改的文件表

- `modules/raft/storage/snapshot_storage.cpp`
  - Phase 4B 的主实现面
  - 负责 temp snapshot dir staging、snapshot data/meta flush、snapshot dir publish、prune durability
- `modules/raft/storage/snapshot_storage.h`
  - 仅在 helper / 注释 / 接口说明确实需要同步时才修改
- `modules/raft/node/raft_node.cpp`
  - 仅在 `SnapshotWorkerLoop` / `OnInstallSnapshot` 的失败暴露、清理或 publish ordering 边界证明不足时最小触碰
- `modules/raft/state_machine/state_machine.cpp`
  - 仅在 state machine 工作文件 durability 被证明是 Phase 4B 闭环必需条件时才修改
- `tests/test_snapshot_storage_reliability.cpp`
- `tests/snapshot_test.cpp`
- `tests/test_raft_snapshot_restart.cpp`
- `tests/test_raft_snapshot_diagnosis.cpp`

## 7. 不应该修改的文件表

- `modules/raft/storage/raft_storage.cpp`
- `modules/raft/storage/raft_storage.h`
- `modules/raft/node/raft_node.h`
- `proto/raft.proto`
- `modules/raft/service/*`
- `modules/raft/replication/*`
- KV 命令语义相关代码
- segment log append / truncate 相关实现
- `meta.bin` / hard state 格式相关实现

## 8. 需要新增或修改的测试

- `tests/test_snapshot_storage_reliability.cpp`
  - direct-to-final-dir 现状测试需要被 staged publish 语义替换
  - 增加 temp snapshot dir 不应被 `ListSnapshots()` 当作 trusted snapshot 的测试
  - 增加缺失 `data.bin` / 缺失 meta / checksum 损坏时的回退测试
  - 增加 publish 后目录布局与 cleanup 测试
- `tests/snapshot_test.cpp`
  - 目录 snapshot 可见性断言需要适配 staged publish 后的最终布局
- `tests/test_raft_snapshot_restart.cpp`
  - 验证 snapshot + post-snapshot tail log 的 restart recovery 在新 publish 语义下不回归
- `tests/test_raft_snapshot_diagnosis.cpp`
  - 验证 startup 继续优先加载最新有效 snapshot，并在无效最新 snapshot 时回退到旧 snapshot

## 9. Phase 4B 最小实现计划

1. 在 `snapshot_storage.cpp` 中把 `SaveSnapshotFile()` 改为 staged temp snapshot dir publish，而不是 direct-to-final-dir。
2. 为 staged `data.bin` 和 `__raft_snapshot_meta` 增加 file `fsync`。
3. 为 staged snapshot dir 和 `snapshot_dir` 的 publish / prune 路径增加 directory `fsync`。
4. 消除 `remove_all(final_dir)` 先删后发窗口；如果某个平台无法对目录替换提供等价保证，必须返回明确错误，不能静默成功。
5. 只在测试证明 `SnapshotWorkerLoop` / `OnInstallSnapshot` 的失败暴露或顺序边界不足时，最小修改 `raft_node.cpp`。
6. 默认不修改 snapshot binary/meta 格式，不重写 storage engine。

## 10. 是否修改持久化格式

否。

Phase 4 当前需要收敛的是 snapshot publish protocol 和 durability barrier，不是 `data.bin` 或 `__raft_snapshot_meta` 的字段、编码或目录命名格式。修改格式会引入兼容性和恢复迁移风险，不属于本阶段目标。

## 11. 跨平台 durability 要求

- POSIX/Linux 使用真实 `fsync`
- Windows 不允许 no-op success
- Windows 文件和目录路径需要遵守已有 contract：文件使用 `FlushFileBuffers`，目录路径如果无法提供等价 publish 语义则必须返回明确错误
- 当前未在 Windows 环境验证 snapshot publish 路径；Windows 运行时语义应标记为“未在 Windows 环境验证”

## 12. 明确确认没有修改业务代码

本阶段只做 snapshot atomic publish 分析、任务补充、进度更新、决策记录和阶段报告。未修改业务代码，未实现 snapshot atomic publish，未修改持久化格式，未修改 Raft 协议，未修改 KV 逻辑，也未删除测试。
