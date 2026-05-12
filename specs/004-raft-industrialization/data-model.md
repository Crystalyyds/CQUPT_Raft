# Data Model: CQUPT_Raft Industrialization Hardening

## 1. CapabilityClass

- **Purpose**: Classify current project behavior so stable baseline work is not
  accidentally replanned.
- **Fields**:
  - `code`: one of `A`, `B`, `C`, `D`, `E`, `F`, `G`
  - `name`: human-readable classification
  - `description`: scope definition
  - `examples`: representative project areas
  - `plan_action`: preserve, test, fix, add, or defer
- **Validation rules**:
  - Every planned task must reference at least one `CapabilityClass`.
  - Class `A` items cannot become implementation tasks unless the task is
    explicitly about diagnostics, tests, or platform hardening around them.

## 2. IndustrializationTask

- **Purpose**: Represent an actionable work item in the implementation plan and
  later `tasks.md`.
- **Fields**:
  - `id`: stable identifier such as `W1`
  - `priority`: one of `P0`, `P1`, `P2`, `P3`, `P4`
  - `goal`: concise outcome statement
  - `capability_class_refs`: linked `CapabilityClass` codes
  - `change_types`: subset of `test`, `prod-code`, `script`, `doc`
  - `affected_modules`: exact modules or files
  - `linux_specific`: boolean
  - `fallback_required`: boolean
  - `fallback_scope`: Windows/macOS expectation when `fallback_required=true`
  - `dependencies`: predecessor tasks
  - `acceptance_criteria`: measurable completion checks
- **Validation rules**:
  - A `prod-code` task must identify the exact module boundary it is allowed to
    modify.
  - A Linux-specific task must define its fallback or deferred platform scope.
  - A task affecting `storage`, `node`, `replication`, or `state_machine` must
    include at least one recovery or consistency acceptance criterion.

## 3. RecoveryScenario

- **Purpose**: Capture crash/restart/catch-up situations that require trusted
  behavior.
- **Fields**:
  - `name`
  - `trigger`: restart, crash-like artifact, lagging follower, leader switch,
    snapshot publish interruption, etc.
  - `persisted_artifacts`: meta, log segments, snapshot metadata, snapshot data,
    retained temp files
  - `trusted_boundary`: the highest state that may be accepted after recovery
  - `expected_result`
  - `validation_scope`: link to `ValidationScope`
- **Validation rules**:
  - Each high-risk `IndustrializationTask` must reference at least one
    `RecoveryScenario`.
  - Scenarios involving exact fsync/rename/prune failure must be marked
    Linux-specific unless equivalent cross-platform evidence exists.

## 4. ValidationScope

- **Purpose**: Describe where and how evidence is valid.
- **Fields**:
  - `name`: `linux-primary`, `linux-specific`, `platform-neutral`, `future-runtime`
  - `entrypoint`: command, preset, or script
  - `guarantee_level`: baseline regression, exact failure evidence, design-only,
    or deferred runtime validation
  - `artifact_retention`: whether failure data must be preserved
- **Validation rules**:
  - Every planned test group or script must map to one `ValidationScope`.
  - `linux-specific` scopes cannot be used to claim Windows/macOS equivalence.

## 5. ValidationEntrypoint

- **Purpose**: Standardize how maintainers run grouped validation.
- **Fields**:
  - `name`
  - `platforms`
  - `command`
  - `coverage_focus`
  - `requires_bash`: boolean
  - `requires_keep_data`: boolean
- **Validation rules**:
  - There must be at least one Linux primary entrypoint and one platform-neutral
    fallback entrypoint.
  - Windows/macOS planning must not depend solely on a Bash-only entrypoint.

## 6. PlatformSupportExpectation

- **Purpose**: Record the intended contract for cross-platform behavior without
  over-claiming runtime parity.
- **Fields**:
  - `area`: build, path handling, sync/flush semantics, rename semantics,
    temporary directory handling, process/signal-based tests
  - `linux_status`
  - `windows_status`
  - `macos_status`
  - `current_evidence`
  - `next_step`
- **Validation rules**:
  - Areas with Linux-specific evidence must have explicit Windows and macOS
    statuses.
  - Any status other than verified must specify fallback behavior or planned
    follow-up.

## Relationships

- `IndustrializationTask` references one or more `CapabilityClass` items.
- `IndustrializationTask` validates one or more `RecoveryScenario` items.
- `RecoveryScenario` executes within one `ValidationScope`.
- `ValidationScope` uses one or more `ValidationEntrypoint` items.
- `PlatformSupportExpectation` constrains `ValidationScope` and task acceptance.

## State Transitions

### IndustrializationTask

`planned -> in-progress -> accepted -> closed`

- `planned`: defined in `plan.md`
- `in-progress`: selected into `tasks.md` and implementation work starts
- `accepted`: acceptance criteria and validation evidence are complete
- `closed`: artifacts, docs, and regression paths are updated

### RecoveryScenario

`identified -> covered-by-existing-tests -> covered-by-targeted-tests -> validated`

- `identified`: recognized during analysis
- `covered-by-existing-tests`: already protected, no new work needed
- `covered-by-targeted-tests`: new test or harness work added
- `validated`: scenario is included in the maintained regression path
