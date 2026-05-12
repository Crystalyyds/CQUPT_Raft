# Feature Specification: CQUPT_Raft Industrialization Hardening

**Feature Branch**: `[004-raft-industrialization]`  
**Created**: 2026-05-11  
**Status**: Draft  
**Input**: User description: "Generate an industrialization specification for the existing CQUPT_Raft project so it can be strengthened toward a more production-ready, cross-platform distributed metadata consistency layer without reimplementing already stable Raft capabilities."

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Trust Restarted State (Priority: P1)

As a core maintainer, I need committed term, vote, log, snapshot, and applied
state to recover consistently after restart or crash-like disk artifacts so the
project can be treated as a trustworthy metadata-layer baseline rather than a
demo that only works in a clean run.

**Why this priority**: Recovery trust is the foundation for every later
industrialization step. If restarted state is ambiguous, all higher-level
cluster behavior remains suspect.

**Independent Test**: This story can be tested independently by executing the
documented Linux recovery validation flow and showing that committed metadata
state is restored correctly after restart and after trusted-state boundary
artifacts.

**Acceptance Scenarios**:

1. **Given** a node or cluster with committed metadata entries and published
   snapshots, **When** the process is restarted, **Then** the restarted state
   reflects the last trusted committed state and continues serving the same
   metadata view.
2. **Given** crash-like publish artifacts such as incomplete log or snapshot
   outputs, **When** restart recovery runs, **Then** the system accepts only
   trusted persisted state and rejects or skips partial artifacts with
   actionable diagnostics.

---

### User Story 2 - Preserve Cluster Consistency Under Lag And Leadership Change (Priority: P2)

As a cluster maintainer, I need leader changes, follower lag, follower restart,
snapshot handoff, and post-snapshot log replay to remain consistent so that the
system can keep replicating metadata safely while nodes stop, restart, or fall
behind.

**Why this priority**: Once restart trust exists, the next operational risk is
cluster divergence during leadership churn and follower recovery.

**Independent Test**: This story can be tested independently by running the
targeted cluster regression suite and confirming that a lagging or restarted
follower catches up correctly and that the cluster still reaches a consistent
committed state after leader transitions.

**Acceptance Scenarios**:

1. **Given** a follower that misses committed entries while offline, **When**
   it rejoins after the leader has continued replicating and compacting state,
   **Then** it catches up through log replay or snapshot handoff without
   violating committed ordering.
2. **Given** a leader change after committed metadata has already been
   replicated, **When** a new leader is elected and new writes continue,
   **Then** previously committed state remains intact and new committed state is
   applied consistently across the surviving cluster.

---

### User Story 3 - Run A Clear Industrialization Validation Flow (Priority: P3)

As a release or platform maintainer, I need one clear primary validation path,
explicit Linux-only test isolation, and visible Windows/macOS follow-up scope
so industrialization progress can be verified repeatedly without guessing which
tests, scripts, or platform assumptions matter.

**Why this priority**: A project cannot be industrialized sustainably if its
validation flow is fragmented, opaque, or silently Linux-only.

**Independent Test**: This story can be tested independently by reviewing the
documented validation entry points and confirming that failures point to a
specific scenario, platform scope, and follow-up expectation.

**Acceptance Scenarios**:

1. **Given** a maintainer running the primary validation path on Linux, **When**
   a regression occurs, **Then** the failing scenario, validation scope, and
   likely diagnosis path are immediately identifiable.
2. **Given** a platform-specific validation gap outside Linux, **When** the
   feature scope is reviewed, **Then** the missing Windows/macOS coverage is
   explicit rather than implicitly treated as complete.

## Current Baseline & Scope Boundaries *(mandatory)*

### Existing Baseline

- The project already has working leader election, log replication, majority
  commit, apply progression, snapshot save/load/install, follower catch-up,
  segmented log persistence, and restart recovery paths.
- The project already has a broad GoogleTest and CTest suite covering election,
  replication, persistence, snapshot, restart, diagnosis, and split-brain
  scenarios.
- The earlier persistence reliability feature already established and partially
  completed targeted work for segmented log durability barriers, hard-state
  persistence barriers, staged snapshot publish, restart diagnostics, and
  crash-like trusted-state artifact coverage.
- These baseline capabilities MUST be preserved and are not to be reimplemented
  by this feature.

### Targeted Gaps Or Risks

- Remaining timing-sensitive Linux validation blockers make the current primary
  recovery and split-brain acceptance path less trustworthy than required.
- Exact failure-injection coverage for sync, rename, prune, and partial-write
  failures is still missing.
- Some implemented areas are stable on Linux-oriented paths but still lack
  Windows/macOS runtime validation or explicit fallback planning.
- Existing recovery and durability work is stronger than before, but the
  project still needs a consolidated industrialization scope that separates
  already finished work from remaining cluster-consistency, validation-flow,
  observability, and platform-readiness gaps.

### Non-Goals

- Reimplementing Raft from scratch.
- Repeating already stable and test-backed consensus behavior as new feature
  work.
- Introducing an unrelated business storage system or changing the current
  project into a full database product.
- Large-scale architectural replacement, naming churn, or rewrite-oriented
  refactors.
- Changing persisted formats, protocol semantics, public API behavior, or the
  verified sample KV state-machine semantics unless a future feature scopes such
  a change explicitly.

### Platform Scope

- Linux is the primary validation environment for this feature.
- Linux-specific crash, failure-injection, process, signal, or durability
  evidence MUST be labeled as Linux-specific.
- Windows and macOS are in scope for design preservation, path semantics,
  build/test entry planning, and future runtime validation tasks, but not for
  claiming full equivalence without explicit follow-up coverage.

### Edge Cases

- What happens when persisted hard state points to a boundary that the on-disk
  log cannot fully justify?
- How does the system respond when the newest snapshot-looking artifact is
  incomplete, corrupted, or only partially published?
- What happens when a follower must recover after falling behind both the live
  log tail and the retained snapshot boundary?
- How is validation reported when Linux-only crash-style tests pass but
  Windows/macOS runtime semantics remain unverified?

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The specification MUST classify current CQUPT_Raft capabilities
  into the following buckets: already complete and not to be handled, complete
  but under-tested, complete but consistency or recovery risky, complete but
  cross-platform risky, partially implemented, missing but required for
  industrialization, and explicitly deferred.
- **FR-002**: The feature scope MUST preserve already stable leader election,
  log replication, majority commit, apply, snapshot, catch-up, and restart
  capabilities as protected baseline behavior rather than new implementation
  deliverables.
- **FR-003**: The specification MUST define the remaining industrialization work
  needed to trust recovery of committed term, vote, log entries, snapshot
  metadata, and applied state after node restart.
- **FR-004**: The specification MUST define the remaining industrialization work
  needed to preserve cluster consistency across follower lag, follower restart,
  leader change, snapshot handoff, and post-snapshot log replay.
- **FR-005**: The specification MUST identify which persistence and recovery
  hardening tasks are already completed under the earlier persistence
  reliability effort and MUST exclude them from duplicate implementation scope
  unless they remain blocked or require final validation.
- **FR-006**: The specification MUST define how crash-like testing, exact
  failure injection, and power-loss approximation are separated into current
  coverage, missing coverage, and future follow-up.
- **FR-007**: The project MUST provide a documented primary validation path for
  Linux that covers build, baseline tests, persistence and restart recovery,
  snapshot behavior, and cluster consistency scenarios relevant to this feature.
- **FR-008**: The project MUST provide actionable failure-location guidance for
  each targeted validation area so maintainers can map a failing scenario to a
  likely subsystem without rereading the entire codebase.
- **FR-009**: Any Linux-specific validation path MUST be explicitly labeled and
  MUST identify the cross-platform fallback or follow-up expectation.
- **FR-010**: Any change affecting persistence, snapshot, restart recovery, or
  replication MUST define the expected behavior under crash, restart, or
  partial-publish conditions that matter to this feature.
- **FR-011**: The specification MUST define the required cross-platform build
  and test entry expectations for Windows and macOS, even if Linux remains the
  only primary execution target in the current phase.
- **FR-012**: The specification MUST allow small-scope engineering additions
  such as new tests, new scripts, diagnostics, validation helpers, and narrow
  abstraction modules when they reduce reliability or platform risk without
  forcing a broad rewrite.

### Key Entities *(include if feature involves data)*

- **Capability Class**: A named classification of current system behavior used
  to separate protected baseline functionality from work that still requires
  industrialization.
- **Recovery Scenario**: A restart, crash-like, or catch-up situation with an
  expected trusted-state outcome and an associated validation path.
- **Validation Scope**: A declared execution context such as Linux primary
  validation, cross-platform design preservation, or deferred runtime coverage.
- **Industrialization Gap**: A missing or weak reliability, observability,
  recovery, or platform-readiness property that still needs planned work.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: The resulting scope explicitly lists protected baseline
  capabilities and identifies zero already stable CQUPT_Raft core behaviors as
  new implementation work items.
- **SC-002**: Every targeted industrialization gap is assigned to one of the
  required capability classes, and each class has at least one acceptance
  scenario or explicit deferred rationale.
- **SC-003**: Maintainers can identify a documented primary Linux validation
  flow and at least one failure-diagnosis path for each of the following areas:
  persistence durability, restart recovery, snapshot behavior, and cluster
  consistency under lag or leadership change.
- **SC-004**: The specification names the current Linux-specific validation
  boundaries and the unresolved Windows/macOS runtime validation scope without
  claiming cross-platform completion where evidence does not yet exist.
- **SC-005**: The remaining industrialization scope explicitly distinguishes
  previously completed persistence-hardening phases from unresolved blockers,
  missing failure injection, flaky validation, and broader cross-platform
  follow-up work.

## Assumptions

- Existing verified Raft behaviors remain the reference baseline and will not be
  intentionally regressed for the sake of refactoring.
- The earlier persistence reliability work remains authoritative for already
  completed durability improvements and is reused as baseline context rather
  than reopened as fresh implementation scope.
- Linux continues to be the only primary execution environment for crash-like
  and durability-oriented validation in the current phase.
- Windows and macOS follow-up planning is required even when runtime execution
  is deferred.
- Necessary engineering additions may include tests, scripts, diagnostics, and
  small supporting abstractions, but not rewrite-scale structural replacement.
