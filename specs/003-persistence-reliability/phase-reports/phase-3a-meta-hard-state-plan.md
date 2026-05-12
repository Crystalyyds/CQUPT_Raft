# Phase 3A Report: Meta Hard State Plan

## 1. `meta.bin` 当前写入路径

当前 `meta.bin` 的写入路径位于 `modules/raft/storage/raft_storage.cpp`，顺序如下：

1. `RaftNode::PersistStateLocked()` 组装 `PersistentRaftState`
2. `storage_->Save(state, reason)`
3. `FileRaftStorage::Save()`
4. `WriteSegments(log.tmp/, state.log, error)`
5. `WriteMeta(meta.bin.tmp, state, error)`
6. `ReplaceDirectory(log.tmp/, log/, log.bak/, error)`
7. `ReplaceFile(meta.bin.tmp, meta.bin, error)`

当前 `meta.bin` 不是单独持久化；它和 `log/` 一起构成 restart recovery 时的可信 persisted raft state。

## 2. hard state 当前持久化调用链

当前 hard state 包括：

- `current_term`
- `voted_for`
- `commit_index`
- `last_applied`

当前通用调用链是：

1. `RaftNode` 在 term / vote / commit / apply 边界更新内存状态
2. 调用 `RaftNode::PersistStateLocked()`
3. `PersistStateLocked()` 复制：
   - `current_term_`
   - `voted_for_`
   - `commit_index_`
   - `last_applied_`
   - `log_`
4. `storage_->Save(state, reason)`
5. `FileRaftStorage::Save()` 将 hard state 编码进 `meta.bin.tmp`
6. `ReplaceFile(meta.bin.tmp, meta.bin, error)` 发布最终 `meta.bin`

当前主要调用面包括：

- higher term 转 follower
- candidate 发起选举并持久化 term / self vote
- granted vote 后持久化 `voted_for`
- append entries 导致 log 或 `commit_index` 变化
- apply committed entries 导致 `last_applied` 变化
- install snapshot / startup snapshot recovery 后推进 `commit_index` / `last_applied`
- stop 时的最终状态落盘

## 3. `WriteMeta` / `ReplaceFile` / `PersistStateLocked` 的职责

### `WriteMeta`

职责：

- 按当前既有格式编码 `meta.bin.tmp`
- 写入：
  - `current_term`
  - `voted_for`
  - `commit_index`
  - `last_applied`
  - `first_log_index`
  - `last_log_index`
  - `log_count`
- 负责 file content 层面的临时文件写入完成

不负责：

- 最终发布 `meta.bin`
- directory durability
- `RaftNode` 状态收集

### `ReplaceFile`

职责：

- 将 `meta.bin.tmp` 发布为 `meta.bin`
- 负责旧 `meta.bin` 删除与新 `meta.bin` rename
- Phase 3B 中应成为 `meta.bin` publish durability 的主要落点

不负责：

- `meta.bin.tmp` 内容生成
- `RaftNode` 状态收集
- segment log publish

### `PersistStateLocked`

职责：

- 从 `RaftNode` 当前内存状态组装 `PersistentRaftState`
- 定义 hard state 与 log 一次持久化提交的调用边界
- 将 storage 失败向上反馈，并记录 `RecordStoragePersistFailure()`

不负责：

- `meta.bin` 编码细节
- 文件发布细节
- 具体平台 flush 实现

## 4. 需要修改的文件表

Phase 3B 最小实现预计只应修改以下文件：

- `modules/raft/storage/raft_storage.cpp`
- `tests/test_raft_segment_storage.cpp`
- `tests/persistence_test.cpp`
- `specs/003-persistence-reliability/tasks.md`
- `specs/003-persistence-reliability/progress.md`
- `specs/003-persistence-reliability/decisions.md`
- `specs/003-persistence-reliability/phase-reports/phase-3a-meta-hard-state-plan.md`

如果后续确实需要在接口层补少量 helper 声明，才允许评估：

- `modules/raft/storage/raft_storage.h`

当前 Phase 3A 结论是：尚未发现必须改 `raft_storage.h` 的证据。

## 5. 不应该修改的文件表

Phase 3 范围外，本阶段不应修改：

- `modules/raft/node/raft_node.cpp`
- `modules/raft/node/raft_node.h`
- `modules/raft/storage/snapshot_storage.cpp`
- `modules/raft/storage/snapshot_storage.h`
- `modules/raft/state_machine/*`
- `proto/raft.proto`
- `modules/raft/service/*`
- `modules/raft/replication/*`
- `CMakeLists.txt`
- `tests/snapshot_test.cpp`
- `tests/test_snapshot_storage_reliability.cpp`
- `tests/test_raft_snapshot_restart.cpp`

备注：

- `PersistStateLocked` 位于 `raft_node.cpp`，但 Phase 3A 当前只做职责分析，不修改它。
- 若 Phase 3B 发现必须修改 `PersistStateLocked`，应保持最小化且只限 hard state publish 语义，不触碰选举、复制、apply、snapshot 语义。

## 6. 需要新增或修改的测试

Phase 3A 的测试设计优先复用现有测试文件：

### `tests/test_raft_segment_storage.cpp`

建议新增或扩展 storage-level 测试：

- `meta.bin` 临时文件发布后的最终文件可见性检查
- `meta.bin` 缺失时 `Load()` 的行为
- `meta.bin` 头部损坏时 `Load()` 的失败行为
- `meta.bin` 与 segment 边界不一致时 `Load()` 的失败行为

### `tests/persistence_test.cpp`

建议新增或扩展 persistence-level 测试：

- term / vote 持久化后 restart recovery 的行为
- `commit_index` / `last_applied` 持久化后 restart recovery 的行为
- `meta.bin` 损坏或缺失时，节点启动恢复的失败或拒绝行为
- Phase 2 segment log 已 durable 时，Phase 3 `meta.bin` publish 仍不足以单独定义可信状态的边界测试

当前 Phase 3A 结论是不新增测试 target，不修改 CMake。

## 7. 测试场景设计

### 7.1 term / vote 写入成功后的 durability 边界

目标：

- 验证当 `current_term` / `voted_for` 被持久化后，restart recovery 读取到的是最新已发布的 `meta.bin`
- 验证 `meta.bin.tmp` 未发布完成时，不应把临时文件当作可信状态

建议测试面：

- storage-level：人工构造 `meta.bin.tmp` 与 `meta.bin` 并验证 `Load()` 只读取最终 `meta.bin`
- persistence-level：触发选举或投票后的 stop/restart，验证 term / vote 恢复行为

### 7.2 `commit_index` / `last_applied` 写入成功后的 durability 边界

目标：

- 验证 `commit_index` / `last_applied` 已发布后，restart recovery 可按当前 contract 恢复到可信边界
- 验证恢复时仍遵守当前逻辑：`commit_index_ = max(commit_index, last_applied)`，`last_applied_` 运行时从 snapshot + replay 重建

建议测试面：

- persistence-level：写入若干 entry、stop/restart，验证 committed data 恢复
- storage-level：直接保存并加载 `PersistentRaftState`，验证 `commit_index` / `last_applied` 编解码一致

### 7.3 `meta.bin` 临时文件发布后的 directory fsync

目标：

- 为 Phase 3B 明确 `meta.bin.tmp -> meta.bin` publish 后需要补 parent directory durability
- 设计测试覆盖最终文件发布与临时文件清理可见性

建议测试面：

- storage-level：保存后确认 `meta.bin` 存在、`meta.bin.tmp` 不应作为可信输入
- 若实现后可在单元测试中观测，则补对 `meta.bin.tmp` 遗留场景的恢复行为断言

### 7.4 `meta.bin` 损坏或缺失时的 restart recovery 行为

目标：

- 验证 `meta.bin` 缺失时，当前 `Load()` 不进入 segmented load
- 验证 `meta.bin` magic/version 损坏时，`Load()` 失败
- 验证 `meta.bin` 与 segment count / boundary 不一致时，恢复失败

建议测试面：

- 直接修改 `meta.bin` 字节
- 删除 `meta.bin`
- 伪造边界不一致的 `meta.bin`

### 7.5 Phase 2 segment log durability 与 Phase 3 meta durability 的边界关系

目标：

- 明确 Phase 2 只保证 segment file 与 `log/` directory publish
- 明确 Phase 3 负责 `meta.bin` file / directory publish
- 验证“新 log + 旧 meta”仍然由旧 meta 边界支配，而不是把新 log 视为已提交可信状态

建议测试面：

- storage-level：保留新 log，回退或伪造旧 meta，验证 `Load()` 仍按 meta 边界做可信判断
- 这类测试不需要改持久化格式，也不应引入 snapshot 路径

## 8. Phase 3B 的最小实现计划

Phase 3B 最小实现应只做以下事情：

1. 在 `WriteMeta()` 中补 file flush helper
2. 在 `ReplaceFile()` 所在的 `meta.bin` publish 路径补 parent directory flush
3. 保持现有 `meta.bin` 编码格式完全不变
4. 不改 `WriteSegments()`、`ReplaceDirectory()`、snapshot publish 或 `KV`
5. 仅在必要时最小化调整 `PersistStateLocked()` 的错误传播边界；默认不改其业务语义
6. 先补或更新 `tests/test_raft_segment_storage.cpp` 与 `tests/persistence_test.cpp`，再做实现

## 9. 是否会修改持久化格式

不会。

Phase 3A 的预期和 Phase 3B 的最小实现计划都不修改持久化格式。

## 10. 跨平台要求

- POSIX/Linux：对 required durability operations 使用真实 `fsync`
- Windows：不允许 no-op success
- Windows：若需要 file flush，应使用 `FlushFileBuffers`
- Windows：若需要 directory flush，应使用目录 handle 方案，失败时返回明确错误
- Windows 当前状态：本阶段只做计划，未在 Windows 环境验证

## 11. 范围检查结果

当前 Phase 3A 没有发现必须扩大范围的阻塞项。

原因：

- `meta.bin` / hard state durability 的 affected files 可以闭合在：
  - `modules/raft/storage/raft_storage.cpp::WriteMeta`
  - `modules/raft/storage/raft_storage.cpp::ReplaceFile`
  - `modules/raft/node/raft_node.cpp::PersistStateLocked`
- 测试设计可以优先复用：
  - `tests/test_raft_segment_storage.cpp`
  - `tests/persistence_test.cpp`

因此当前不需要扩大到 snapshot、segment Phase 2 实现或持久化格式修改。

## 12. 本阶段确认

- 本阶段不修改业务代码
- 本阶段不实现 `fsync`
- 本阶段不修改持久化格式
- 本阶段不修改 Raft 协议
- 本阶段不修改 KV
- 本阶段不删除测试
- 本阶段不创建 task 子目录
