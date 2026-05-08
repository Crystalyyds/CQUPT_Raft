# Phase 2 Report: Storage Log Fsync

## 本阶段范围

本阶段只处理以下内容：

- segment log append 的 file `fsync`
- segment log truncate 的 file `fsync`
- `log/` directory publish / replace 的 directory `fsync`
- `WriteSegments`
- `ReplaceDirectory`
- segment log append / truncate / recovery 相关测试

本阶段未处理：

- `meta.bin`
- hard state
- `WriteMeta`
- `ReplaceFile`
- `PersistStateLocked`
- `modules/raft/node`
- snapshot publish
- KV 逻辑
- proto / RPC
- 持久化格式变更

## 代码修改摘要

### `modules/raft/storage/raft_storage.cpp`

- POSIX/Linux 路径为 segment file 写入补充了 `fsync`
- POSIX/Linux 路径为空 `log.tmp/` 或已写入完成的 `log.tmp/` 补充了 directory `fsync`
- POSIX/Linux 路径为 `log.tmp/ -> log/` publish 后的父目录补充了 directory `fsync`
- POSIX/Linux 路径为 tail truncate 路径补充了 truncate 后的 file `fsync`
- Windows file flush 使用 `CreateFileW` + `FlushFileBuffers`
- Windows directory flush 使用 `CreateFileW` + `FILE_FLAG_BACKUP_SEMANTICS` + `FlushFileBuffers`
- Windows directory flush 若失败会直接返回错误，不再静默成功
- 保持现有持久化格式、segment checksum、meta layout 和对外接口不变

### `tests/test_raft_segment_storage.cpp`

- 新增 `TruncatesCorruptedSegmentTailDuringRecovery`
  - 验证 segment 尾部损坏时，恢复路径会截断坏尾部并保持可加载
- 新增 `SaveDoesNotLeaveTemporaryOrBackupLogDirectories`
  - 验证多次保存和 compaction save 后不会遗留 `log.tmp` / `log.bak`

## 测试执行

执行了以下测试：

- `cmake --build --preset debug-ninja-low-parallel --target test_raft_segment_storage persistence_test`
- `ctest --test-dir build --output-on-failure -R "RaftSegmentStorageTest|PersistenceTest"`
- `CTEST_PARALLEL_LEVEL=1 ./test.sh --group persistence`

结果：

- `RaftSegmentStorageTest` 5 个相关用例全部通过
- `PersistenceTest` 2 个用例全部通过
- `./test.sh --group persistence` 通过

## 结果与边界确认

- 本阶段没有修改持久化格式
- 本阶段没有修改 `modules/raft/node`
- 本阶段没有修改 `meta.bin` publish 路径
- 本阶段没有修改 snapshot publish 路径
- 本阶段只对 segment log file / directory durability 做了最小实现
- `_WIN32` no-op flush 分支已移除

## 平台验证状态

- POSIX/Linux：已在当前环境构建并运行相关测试
- Windows：代码路径已补齐 `FlushFileBuffers` / directory handle 方案
- Windows 当前状态：未在 Windows 环境验证

## 未解决问题

- `meta.bin` 的 file `fsync` 与 publish durability 仍未处理
- hard state 与 `meta.bin` 的一致 publish 仍未处理
- snapshot publish 的原子性与 directory durability 仍未处理
- recovery diagnostics 仍未增强

这些内容应进入 Phase 3 及之后的阶段。

## Phase 3 建议入口

- `modules/raft/storage/raft_storage.cpp::WriteMeta`
- `modules/raft/storage/raft_storage.cpp::ReplaceFile`
- `modules/raft/node/raft_node.cpp::PersistStateLocked`

原因：

- 这些位置共同定义 `meta.bin` 与 hard state 的 durable publish 边界
