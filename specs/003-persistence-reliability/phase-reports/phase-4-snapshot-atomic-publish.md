# Phase 4B: Snapshot Atomic Publish Minimal Implementation

## Scope

Phase 4B implemented staged snapshot publish for `FileSnapshotStorage` only. The final on-disk layout remains compatible:

- `snapshot_<index>/data.bin`
- `snapshot_<index>/__raft_snapshot_meta`

No snapshot data format, metadata format, directory naming format, Raft protocol, KV business semantics, segment log logic, or hard state logic was changed.

## Files Changed

- `modules/raft/storage/snapshot_storage.cpp`
  - Added internal `SyncFile()` and `SyncDirectory()` helpers.
  - POSIX uses real `fsync`.
  - Windows uses `FlushFileBuffers`; directory flush uses a directory handle opened with `FILE_FLAG_BACKUP_SEMANTICS`.
  - Changed `SaveSnapshotFile()` from direct-to-final-dir writes to staged temp snapshot dir publish.
  - Added file fsync for `data.bin`.
  - Added file fsync for `__raft_snapshot_meta`.
  - Added directory fsync for staging dir, `snapshot_dir`, and prune deletion.
  - Changed same index / same term save to idempotent success when an existing valid snapshot is already present.
  - Same index / different term existing valid snapshot returns an explicit error instead of replacing a trusted directory.

- `modules/raft/storage/snapshot_storage.h`
  - Updated the `SaveSnapshotFile()` comment so it no longer documents direct-to-final-dir publishing.

- `tests/test_snapshot_storage_reliability.cpp`
  - Replaced the direct-publish expectation with final-layout compatibility checks.
  - Added coverage for staging dir ignore behavior.
  - Added coverage for missing data, missing metadata, corrupted checksum, fallback to older valid snapshot, and same-index idempotence.

- `specs/003-persistence-reliability/tasks.md`
- `specs/003-persistence-reliability/progress.md`
- `specs/003-persistence-reliability/decisions.md`
- `specs/003-persistence-reliability/phase-reports/phase-4-snapshot-atomic-publish.md`

## Implementation Notes

`SaveSnapshotFile()` now writes into a staging directory named with `.snapshot_staging_...`. The staging directory is not accepted by `ListSnapshots()` because trusted directory snapshots must start with `snapshot_` and pass metadata/data/checksum validation.

The publish sequence is:

1. Create and fsync `snapshot_dir`.
2. Create staging directory and fsync `snapshot_dir`.
3. Copy snapshot data to staging `data.bin`, close it, and fsync the file.
4. Write staging `__raft_snapshot_meta`, close it, and fsync the file.
5. Fsync the staging directory.
6. Rename staging directory to final `snapshot_<index>/`.
7. Fsync `snapshot_dir`.

If a valid final snapshot with the same index and term already exists, the save is treated as idempotent and the existing snapshot is returned. This avoids deleting a trusted snapshot before the replacement has a portable atomic directory swap available.

If an existing final directory is invalid or incomplete, it is not trusted by recovery. Phase 4B may remove that invalid directory after staging is complete, fsync `snapshot_dir`, and then publish the staged directory.

## Recovery Behavior

`ListSnapshots()` still accepts only valid snapshots:

- Directory name starts with `snapshot_`.
- Metadata header and version are valid.
- Directory index matches metadata index.
- `data.bin` exists.
- Checksum matches.

This preserves the trusted-state rule: newest valid snapshot first, invalid snapshots skipped, fallback to an older valid snapshot.

## Tests

Ran:

- `cmake --build --preset debug-ninja-low-parallel --target test_snapshot_storage_reliability`
- `ctest --test-dir build --output-on-failure -R "SnapshotStorageReliabilityTest"`
- `cmake --build --preset debug-ninja-low-parallel --target snapshot_test test_raft_snapshot_restart test_raft_snapshot_diagnosis`
- `ctest --test-dir build --output-on-failure -R "RaftSnapshotRecoveryTest|RaftSnapshotRestartTest|RaftSnapshotDiagnosisTest"`
- `CTEST_PARALLEL_LEVEL=1 ./test.sh --group persistence`

Results:

- `SnapshotStorageReliabilityTest`: 6/6 passed
- Snapshot recovery / restart / diagnosis tests: 7/7 passed
- Persistence group: 3/3 passed

## Cross-Platform Status

- POSIX/Linux: implemented with real `fsync` and verified in the current environment.
- Windows: code path uses `FlushFileBuffers` and does not contain no-op success for required snapshot durability operations.
- Windows runtime behavior was not verified in this environment.

## Explicit Non-Changes

- Did not modify `modules/raft/node/raft_node.cpp`.
- Did not modify `modules/raft/state_machine/state_machine.cpp`.
- Did not modify segment log append / truncate.
- Did not modify `WriteSegments` or `ReplaceDirectory`.
- Did not modify `meta.bin`, hard state, `WriteMeta`, `ReplaceFile`, or `PersistStateLocked`.
- Did not modify proto / RPC / KV / transport.
- Did not modify snapshot data or metadata persistence format.
- Did not delete or skip tests.

## Remaining Issues

- Windows branch still needs real Windows CI or machine validation.
- Same index / different term replacement now returns an explicit error because portable non-empty directory atomic replacement is not available in the current implementation.
- Crash / failure injection for snapshot publish windows is still Phase 6 scope.

## Phase 5 Suggested Entry

- `RaftNode::LoadLatestSnapshotOnStartup()`
- `FileSnapshotStorage::ListSnapshots()`
- `FileSnapshotStorage::ReadDirectorySnapshot()`
- snapshot restart / diagnosis tests that assert trusted-state selection and recovery diagnostics
