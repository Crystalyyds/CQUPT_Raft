# Phase 6B: Crash / Failure Injection Tests

## 1. Summary

Phase 6B implemented the minimum crash artifact test coverage that can be achieved without modifying production code. The added tests simulate crash / power loss approximation by constructing leftover temp files, backup directories, crossed meta/log publish states, partial segment tails, and invalid snapshot catalogs.

This phase did not introduce a test-only failure injection hook. Exact `fsync`, directory `fsync`, rename / replace, remove / prune, and partial write failure injection remains unresolved because those points cannot be deterministically controlled from tests without a production-code hook.

## 2. Modified Files

| File | Why |
| --- | --- |
| `tests/test_raft_segment_storage.cpp` | Added crash artifact tests for partial segment header truncate, leftover `meta.bin.tmp` / `log.tmp` / `log.bak`, and crossed meta/log publish windows |
| `tests/test_snapshot_storage_reliability.cpp` | Added all-invalid snapshot catalog test to verify no untrusted snapshot is selected and diagnostics are preserved |
| `specs/003-persistence-reliability/tasks.md` | Updated Phase 6B checklist status and marked exact injection tasks as deferred |
| `specs/003-persistence-reliability/progress.md` | Recorded Phase 6B completion state, tests run, and unresolved injection hook gap |
| `specs/003-persistence-reliability/decisions.md` | Recorded decisions to avoid production hooks in Phase 6B and defer exact injection |
| `specs/003-persistence-reliability/phase-reports/phase-6-crash-failure-injection-tests.md` | This report |

## 3. New / Updated Test Scenarios

### Segment log / meta crash artifacts

- `RaftSegmentStorageTest.TruncatesPartialSegmentHeaderDuringRecovery`
  - Appends a one-byte partial segment entry header to the tail segment.
  - Verifies recovery truncates the bad tail and preserves the trusted log.

- `RaftSegmentStorageTest.RecoveryIgnoresTemporaryPublishArtifacts`
  - Creates leftover `meta.bin.tmp`, `log.tmp/`, and `log.bak/` after a successful save.
  - Verifies load ignores these incomplete publish artifacts and trusts only `meta.bin + log/`.

- `RaftSegmentStorageTest.MetaAndLogPublishWindowUsesOnlyTrustedBoundary`
  - Simulates old `meta.bin` with new `log/`.
  - Verifies recovery is bounded by old trusted meta state.
  - Simulates new `meta.bin` with old `log/`.
  - Verifies recovery rejects the state with `log count mismatch`.

### Snapshot crash artifacts

- `SnapshotStorageReliabilityTest.AllInvalidSnapshotsReturnNoTrustedSnapshotWithDiagnostics`
  - Creates a catalog containing staging-only, missing-meta, missing-data, and checksum-mismatched snapshots.
  - Verifies `ListSnapshotsWithDiagnostics()` returns no trusted snapshot and reports skip reasons.
  - Verifies `LoadLatestValidSnapshot()` returns `has_snapshot=false`.

## 4. Crash Matrix Covered In Phase 6B

| Area | Crash artifact | Expected recovery behavior | Test |
| --- | --- | --- | --- |
| segment log | partial tail header | truncate tail, keep trusted records | `TruncatesPartialSegmentHeaderDuringRecovery` |
| storage publish | leftover `meta.bin.tmp` | ignore temp meta, trust complete `meta.bin` | `RecoveryIgnoresTemporaryPublishArtifacts` |
| storage publish | leftover `log.tmp/` | ignore temp log dir, trust final `log/` | `RecoveryIgnoresTemporaryPublishArtifacts` |
| storage publish | leftover `log.bak/` | ignore backup dir, trust final `log/` | `RecoveryIgnoresTemporaryPublishArtifacts` |
| meta/log ordering | old meta + new log | recover only old meta boundary | `MetaAndLogPublishWindowUsesOnlyTrustedBoundary` |
| meta/log ordering | new meta + old log | reject untrusted boundary mismatch | `MetaAndLogPublishWindowUsesOnlyTrustedBoundary` |
| snapshot catalog | staging dir only | skip staging dir | `AllInvalidSnapshotsReturnNoTrustedSnapshotWithDiagnostics` |
| snapshot catalog | missing `__raft_snapshot_meta` | skip invalid snapshot | `AllInvalidSnapshotsReturnNoTrustedSnapshotWithDiagnostics` |
| snapshot catalog | missing `data.bin` | skip invalid snapshot | `AllInvalidSnapshotsReturnNoTrustedSnapshotWithDiagnostics` |
| snapshot catalog | checksum mismatch | skip invalid snapshot | `AllInvalidSnapshotsReturnNoTrustedSnapshotWithDiagnostics` |
| snapshot catalog | all snapshots invalid | return no trusted snapshot | `AllInvalidSnapshotsReturnNoTrustedSnapshotWithDiagnostics` |

## 5. Deferred Failure Injection

The following Phase 6B tasks remain intentionally deferred:

- T613: test-only failure injection helper.
- T614: exact segment log `SyncFile`, `ReplaceDirectory`, cleanup, and directory sync failure tests.
- T615: exact `WriteMeta` / `ReplaceFile` file and directory sync failure tests.
- T616: exact snapshot staged data/meta sync, staging dir sync, publish rename, parent dir sync, prune remove/sync failure tests.

Reason:

- These failures happen inside production storage helpers.
- Current tests can construct before/after disk states, but cannot force a specific internal syscall or filesystem operation to fail.
- Implementing deterministic injection requires a production-visible test hook or test-only compiled hook.
- Phase 6B did not modify production code, so these are not claimed as covered.

## 6. Tests Run

Commands run:

```bash
cmake --build --preset debug-ninja-low-parallel
./build/tests/test_raft_segment_storage --gtest_filter='RaftSegmentStorageTest.TruncatesPartialSegmentHeaderDuringRecovery:RaftSegmentStorageTest.RecoveryIgnoresTemporaryPublishArtifacts:RaftSegmentStorageTest.MetaAndLogPublishWindowUsesOnlyTrustedBoundary'
./build/tests/test_snapshot_storage_reliability --gtest_filter='SnapshotStorageReliabilityTest.AllInvalidSnapshotsReturnNoTrustedSnapshotWithDiagnostics:SnapshotStorageReliabilityTest.IgnoresStagingAndIncompleteSnapshotDirectories:SnapshotStorageReliabilityTest.FallsBackToOlderSnapshotWhenNewestIsCorrupted'
./build/tests/test_raft_segment_storage
./build/tests/test_snapshot_storage_reliability
./build/tests/persistence_test
./build/tests/test_raft_snapshot_restart
./build/tests/test_raft_snapshot_diagnosis
```

Results:

- Build passed.
- `test_raft_segment_storage`: 14 tests passed.
- `test_snapshot_storage_reliability`: 8 tests passed.
- `persistence_test`: 4 tests passed.
- `test_raft_snapshot_restart`: 4 tests passed.
- `test_raft_snapshot_diagnosis`: 2 tests passed.

One intermediate run of the new snapshot test failed because the test constructed an invalid meta file and expected a missing-data diagnostic. The test was corrected to create a valid snapshot and then remove `data.bin`; the rerun passed.

## 7. Format / Protocol / Semantics

- Production code was not modified in Phase 6B.
- Persistent file formats were not modified.
- Raft protocol, RPC/proto, KV logic, transport, and dynamic membership were not touched.
- Phase 2 segment log fsync semantics were not changed.
- Phase 3 meta hard state fsync semantics were not changed.
- Phase 4 snapshot atomic publish semantics were not changed.
- Phase 5 restart recovery behavior and diagnostics semantics were not changed.

## 8. Platform Verification

Tests were run on the current POSIX/Linux environment. Windows runtime behavior was not verified in this phase. No new Windows-specific code path was added.

## 9. Unresolved Issues

- Exact internal durability operation failures still need deterministic test-only injection.
- Phase 6B does not prove behavior for actual power loss or kernel-level I/O errors.
- Permission-based failure simulation was not used because it is platform-specific and can produce brittle tests.

## 10. Phase 6C Suggested Entry

If continuing, Phase 6C should start with a narrow test-only failure injection hook design:

- Define checkpoint names for storage durability operations.
- Keep the hook default-off and test-only.
- Cover `SyncFile`, `SyncDirectory`, rename / replace, remove / prune, write / copy failure.
- Ensure injected failures are returned through existing error channels.
- Do not change persistent formats or production default behavior.
