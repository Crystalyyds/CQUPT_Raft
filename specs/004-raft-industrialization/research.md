# Phase 0 Research: CQUPT_Raft Industrialization Hardening

## Decision 1: Treat the existing Raft core and `003-persistence-reliability` output as protected baseline

- **Decision**: Do not reschedule stable election, replication, commit/apply,
  snapshot save/load/install, follower catch-up baseline, segmented storage, or
  completed `003` durability phases as new implementation work.
- **Rationale**: The codebase already implements these paths and has broad test
  evidence. Reopening them as fresh design scope would violate the constitution
  and create churn instead of reducing risk.
- **Alternatives considered**:
  - Rebuild the plan from a clean-slate Raft model.
    - Rejected because it duplicates completed work and obscures the remaining
      industrialization gaps.

## Decision 2: Stabilize the current Linux validation path before expanding new high-risk coverage

- **Decision**: Make Final Linux Validation trustable first by addressing the
  known flaky snapshot-recovery and split-brain acceptance paths without
  weakening their assertions.
- **Rationale**: New failure-injection or recovery work cannot be validated
  confidently while the primary Linux acceptance path is already unstable.
- **Alternatives considered**:
  - Add new failure-injection tests immediately and defer flaky stabilization.
    - Rejected because noisy baseline failures would hide the signal from new
      recovery and durability tests.

## Decision 3: Add exact durability failure injection through narrow test-only seams

- **Decision**: Introduce opt-in, test-only failure injection around file sync,
  directory sync, replace/rename, prune/remove, and partial-write boundaries in
  `modules/raft/storage`, reusing the smallest internal helper surface that can
  produce deterministic failures.
- **Rationale**: Current tests cover many trusted-artifact constructions but do
  not hit exact persistence operation failures. Exact injection is required to
  validate remaining crash windows and error propagation.
- **Alternatives considered**:
  - Keep using only malformed-file and leftover-artifact construction.
    - Rejected because it cannot prove behavior at exact durability boundaries.
  - Build a fully mocked filesystem layer across the repository.
    - Rejected because it is too invasive and conflicts with the minimal-change
      constraint.

## Decision 4: Prefer tests-first for `node`, `replication`, and `state_machine`

- **Decision**: For catch-up, leader switch, and apply/restart consistency,
  start with targeted regression tests and only modify production code after a
  real defect is reproduced.
- **Rationale**: These areas are already implemented. The plan should prove the
  remaining risk before increasing production-code surface.
- **Alternatives considered**:
  - Preemptively refactor the orchestration code to "make it cleaner".
    - Rejected because it would expand risk without first demonstrating a
      correctness gap.

## Decision 5: Use CTest as the platform-neutral execution contract, keep `test.sh` as Linux primary

- **Decision**: Preserve `test.sh` as the main Linux regression entry while
  treating `ctest --preset` as the platform-neutral baseline and planning a
  PowerShell entry for Windows/macOS environments.
- **Rationale**: The repository already has a working Linux test runner and
  basic presets. Industrialization should extend this rather than replacing it.
- **Alternatives considered**:
  - Replace all test execution with Bash-only orchestration.
    - Rejected because it hardens Linux-only coupling.
  - Remove `test.sh` and rely only on raw `ctest`.
    - Rejected because the grouped Linux workflow and data-retention behavior
      are already useful operational assets.

## Decision 6: Keep Linux-specific crash-style evidence explicit and isolated

- **Decision**: Any test depending on Linux-specific process, signal, directory
  fsync, or exact crash-style semantics must be labeled Linux-specific and
  paired with documented Windows/macOS fallback or follow-up expectations.
- **Rationale**: The project targets cross-platform design, but only Linux
  currently offers the primary validation environment for these scenarios.
- **Alternatives considered**:
  - Treat Linux crash tests as implicit evidence for all platforms.
    - Rejected because rename, flush, directory durability, and process
      semantics differ across platforms.

## Decision 7: Improve failure localization through contracts, grouping, and retained artifacts instead of API changes

- **Decision**: Put failure-location guidance into test grouping, validation
  contracts, log-retention usage, and documentation instead of changing public
  APIs or protocol surfaces.
- **Rationale**: The main problem is diagnosability for maintainers, not lack
  of a new runtime API.
- **Alternatives considered**:
  - Add new external status endpoints or public diagnostics APIs.
    - Rejected because the current feature is industrialization of the existing
      metadata layer, not a public observability API redesign.
