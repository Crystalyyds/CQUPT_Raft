# Quickstart: CQUPT_Raft Industrialization Hardening

## 1. Read the scope boundary first

- Protected baseline and remaining gaps: [spec.md](./spec.md)
- Implementation sequence and acceptance: [plan.md](./plan.md)
- Research decisions: [research.md](./research.md)

## 2. Build the current Linux baseline

```bash
cmake --preset debug-ninja-low-parallel
cmake --build --preset debug-ninja-low-parallel
```

## 3. Run the primary Linux validation path

```bash
CTEST_PARALLEL_LEVEL=1 ./test.sh --group all
```

If a failure must retain artifacts:

```bash
CTEST_PARALLEL_LEVEL=1 ./test.sh --group all --keep-data
```

## 4. Re-run focused regression areas during implementation

```bash
./test.sh --group persistence
./test.sh --group snapshot-recovery
./test.sh --group diagnosis
./test.sh --group snapshot-catchup
./test.sh --group replicator
```

## 5. Use the platform-neutral fallback entry

```bash
ctest --preset debug-tests --output-on-failure
```

This is the expected cross-platform baseline entry. Linux-only crash-style or
failure-injection groups must be documented separately and not implied here.

## 6. Implementation order

1. Freeze capability classification and protect completed `003` work.
2. Stabilize current Linux flaky validation scenarios.
3. Add exact failure injection for durability boundaries.
4. Complete restart trusted-state recovery coverage.
5. Strengthen catch-up, leader-switch, and apply/restart consistency coverage.
6. Consolidate CTest, Bash, and planned PowerShell entrypoints.
7. Finish failure localization and platform-scope documentation.

## 7. Platform notes

- Linux is the primary validation platform for this feature.
- Windows/macOS must get explicit fallback entrypoints and scope notes.
- Do not claim cross-platform crash-semantics equivalence without runtime
  evidence.
