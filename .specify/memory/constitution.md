<!--
Sync Impact Report
Version change: template -> 1.0.0
Modified principles:
- template principle 1 -> I. Preserve The Verified Core
- template principle 2 -> II. Durability Contract Before Convenience
- template principle 3 -> III. Recovery And Consistency First
- template principle 4 -> IV. Cross-Platform By Default, Linux By Primary Validation
- template principle 5 -> V. Observability And Minimal Surface Change
Added sections:
- Engineering Boundaries
- Planning And Quality Gates
Removed sections:
- None
Templates requiring updates:
- ✅ .specify/templates/plan-template.md
- ✅ .specify/templates/spec-template.md
- ✅ .specify/templates/tasks-template.md
- ⚠ pending (not present in repo): .specify/templates/commands/*.md
Follow-up TODOs:
- None
-->
# CQUPT_Raft Constitution

## Core Principles

### I. Preserve The Verified Core
CQUPT_Raft is an existing C++ Raft project with verified election, replication,
snapshot, catch-up, and restart-recovery paths. All future planning MUST start
from an explicit capability baseline and MUST treat already implemented,
stable, and test-backed behavior as protected scope. Work MUST industrialize
gaps, risks, and weak guarantees instead of replanning or rewriting proven
paths. Big-bang rewrites, architecture resets, and speculative redesigns are
prohibited unless the feature explicitly justifies them and preserves existing
behavior with regression evidence.

Rationale: the project has moved beyond a greenfield demo; momentum now comes
from controlled strengthening, not restarts.

### II. Durability Contract Before Convenience
Any work touching `modules/raft/storage`, snapshot publish, restart recovery,
or related file I/O MUST define its durability contract before implementation.
Required durability operations MUST not silently degrade across platforms.
When Linux, Windows, and macOS cannot provide equivalent guarantees, the code
MUST either provide an explicit fallback contract or return a clear error. No
change may alter persisted formats, checksums, publish ordering semantics, or
recovery order unless the feature explicitly scopes migration, compatibility,
and regression coverage.

Rationale: log persistence, snapshot publish, and crash consistency are core
product properties, not optional implementation details.

### III. Recovery And Consistency First
Any change touching `node`, `replication`, `storage`, `state_machine`, `proto`,
or recovery orchestration MUST preserve leader election safety, term and vote
recovery, log matching, majority commit, commit/apply ordering, snapshot
install semantics, follower catch-up, and restart recovery. Such work MUST add
or update targeted tests for the normal path and the relevant failure,
recovery, or restart path. Linux-specific crash or failure-injection tests are
allowed, but they MUST be clearly isolated and MUST not be the only evidence
for cross-platform correctness.

Rationale: consistency bugs usually surface at boundaries between steady-state
replication and recovery behavior.

### IV. Cross-Platform By Default, Linux By Primary Validation
Linux is the primary development and validation environment, but repository
design MUST remain cross-platform. New code MUST prefer `std::filesystem`,
portable path handling, and generator-agnostic CMake. Linux-specific scripts,
signals, process control, `fsync`, and failure-injection mechanisms MUST be
isolated, labeled, and paired with either platform-neutral validation,
Windows/macOS adaptation tasks, or explicit deferred follow-up. Hard-coded
Linux-only paths, directory semantics, or shell assumptions in shared code are
prohibited.

Rationale: the product target is a cross-platform consistency layer, even when
full crash semantics are first validated on Linux.

### V. Observability And Minimal Surface Change
Industrialization work MUST improve diagnosability and control blast radius.
Changes MUST prefer local `.cpp` helpers, narrow abstractions, and
module-boundary-preserving refactors over public API changes, renames, or broad
reorganization. New tests, scripts, validation tools, small supporting modules,
and observability improvements are encouraged when they reduce operational
risk. Structured errors, recovery diagnostics, metrics, and reproducible test
artifacts are mandatory for new high-risk work.

Rationale: maintainability comes from better evidence and narrower changes, not
from cosmetic churn.

## Engineering Boundaries

- Business logic, protocol semantics, persisted formats, public API behavior,
  class names, function names, and namespaces MUST remain unchanged unless the
  feature explicitly scopes and justifies such a change.
- Header files MUST remain interface-oriented; complex logic, I/O, platform
  behavior, and recovery sequencing MUST stay in `.cpp` files.
- High-risk areas include `proto/`, `modules/raft/node`,
  `modules/raft/replication`, `modules/raft/storage`, snapshot/restart/catch-up
  tests, and crash-recovery paths. Any change there MUST minimize affected
  files and document the impacted invariants.
- New modules are allowed only when they clarify a true boundary such as
  durability abstraction, platform adaptation, diagnostics, or test harnesses.
  They MUST not become a disguised rewrite of the existing architecture.

## Planning And Quality Gates

- Every spec and plan MUST distinguish:
  - already stable capabilities that are out of implementation scope,
  - implemented but weakly tested areas,
  - implemented but risky areas,
  - missing industrialization capabilities.
- Every plan touching durability, recovery, snapshot, replication, or
  cross-platform behavior MUST state:
  - affected modules and invariants,
  - crash or restart windows under consideration,
  - Linux-specific validation scope,
  - Windows/macOS fallback or adaptation expectations,
  - concrete test entry points through CTest and any platform-specific scripts.
- Every task set for high-risk work MUST include regression coverage for the
  directly affected behavior and MUST include artifact or log collection steps
  when failures would otherwise be difficult to diagnose.
- Plans and tasks MUST prefer incremental, independently verifiable phases.
  Stable completed capabilities MUST not be scheduled again unless the work is
  explicitly about strengthening reliability, maintainability, observability,
  or cross-platform support around them.

## Governance

This constitution overrides default Spec Kit assumptions when they conflict
with CQUPT_Raft's current state as an existing industrialization effort.
Amendments MUST be made through an explicit constitution update and MUST keep
the dependent templates in sync. Constitution versioning follows semantic
versioning: MAJOR for incompatible governance changes, MINOR for new principles
or materially expanded constraints, and PATCH for clarifications that do not
change intent. Every future specification, implementation plan, task list, and
review for high-risk work MUST include a constitution compliance check against
the principles above.

**Version**: 1.0.0 | **Ratified**: 2026-05-11 | **Last Amended**: 2026-05-11
