# Contract: Validation Entrypoints

## Purpose

Define the maintainer-facing execution contract for industrialization validation
without changing CQUPT_Raft protocol, persistence format, or public APIs.

## Entrypoint Classes

### 1. Linux Primary Entrypoint

- **Current baseline**:
  - `cmake --preset debug-ninja-low-parallel`
  - `cmake --build --preset debug-ninja-low-parallel`
  - `CTEST_PARALLEL_LEVEL=1 ./test.sh --group all`
- **Contract**:
  - Remains the primary acceptance path for Linux.
  - May include Linux-specific grouped tests and `--keep-data` support.
  - Must not be the only documented entrypoint for non-Linux platforms.

### 2. Platform-Neutral CTest Entrypoint

- **Current baseline**:
  - `ctest --preset debug-tests --output-on-failure`
- **Contract**:
  - Becomes the common fallback entrypoint for Windows/macOS planning.
  - Must cover the platform-neutral subset of regression scenarios.
  - Must clearly distinguish Linux-only groups from platform-neutral groups.

### 3. Windows/macOS Wrapper Entrypoint

- **Planned baseline**:
  - PowerShell wrapper or equivalent documented non-Bash command sequence
- **Contract**:
  - Must not assume Bash, `/tmp`, or Linux-only cleanup semantics.
  - Must point to the same core CMake/CTest flow as the platform-neutral path.
  - May exclude Linux-specific crash/failure-injection groups, but the exclusion
    must be explicit.

## Test Grouping Contract

- Group names must map to a clear subsystem focus such as:
  - persistence
  - snapshot-recovery
  - diagnosis
  - snapshot-catchup
  - replicator
  - segment-cluster
- Linux-specific groups must be visibly labeled.
- A failing group must let a maintainer infer the likely area:
  - `storage`
  - `snapshot`
  - `node`
  - `replication`
  - `state_machine`

## Failure Artifact Contract

- Linux primary entrypoints must support preserved failure artifacts through
  `RAFT_TEST_KEEP_DATA=1` or `./test.sh --keep-data`.
- Documentation must describe where retained artifacts live and which scenarios
  rely on them.
- Platform-neutral entrypoints may omit artifact retention only if the
  documentation says how to re-run the same scenario with retention enabled on
  Linux.

## Non-Goals

- No new public API surface for validation.
- No protocol or persistence format changes.
- No promise that Windows/macOS provide the same crash-style evidence as Linux
  until runtime validation exists.
