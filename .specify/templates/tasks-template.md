---

description: "Task list template for feature implementation"
---

# Tasks: [FEATURE NAME]

**Input**: Design documents from `/specs/[###-feature-name]/`
**Prerequisites**: plan.md (required), spec.md (required for user stories), research.md, data-model.md, contracts/

**Tests**: The examples below include test tasks. Tests are OPTIONAL - only include them if explicitly requested in the feature specification.

**Organization**: Tasks are grouped by user story to enable independent implementation and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- Include exact file paths in descriptions

## Path Conventions

- CQUPT_Raft source modules live under `modules/raft/`
- Protocol definitions live under `proto/`
- Entrypoints live under `apps/`
- Tests live under `tests/`
- Spec artifacts live under `specs/`
- Build and test entrypoints may touch `CMakeLists.txt`, `CMakePresets.json`,
  `tests/CMakeLists.txt`, `test.sh`, and platform-specific helper scripts

<!-- 
  ============================================================================
  IMPORTANT: The tasks below are SAMPLE TASKS for illustration purposes only.
  
  The /speckit-tasks command MUST replace these with actual tasks based on:
  - User stories from spec.md (with their priorities P1, P2, P3...)
  - Feature requirements from plan.md
  - Entities from data-model.md
  - Endpoints from contracts/
  
  Tasks MUST be organized by user story so each story can be:
  - Implemented independently
  - Tested independently
  - Delivered as an MVP increment
  
  DO NOT keep these sample tasks in the generated tasks.md file.
  ============================================================================
-->

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Project initialization and scoped repo plumbing

- [ ] T001 Confirm affected CQUPT_Raft modules and target tests from plan.md
- [ ] T002 Update build, preset, or test-entry wiring needed for this feature
- [ ] T003 [P] Add or update feature-specific test data and diagnostics paths

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core infrastructure that MUST be complete before ANY user story can be implemented

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

Examples of foundational tasks for this repository (adjust per feature):

- [ ] T004 Establish durability, recovery, and invariant assumptions in the touched modules
- [ ] T005 [P] Add shared diagnostics, logging, or metrics support required by the feature
- [ ] T006 [P] Add cross-platform path, filesystem, or test-entry helpers required by the feature
- [ ] T007 Add baseline regression tests for the existing stable behavior this feature must preserve
- [ ] T008 Record Linux-specific validation scope and non-Linux fallback or follow-up tasks

**Checkpoint**: Foundation ready - user story implementation can now begin in parallel

---

## Phase 3: User Story 1 - [Title] (Priority: P1) 🎯 MVP

**Goal**: [Brief description of what this story delivers]

**Independent Test**: [How to verify this story works on its own]

### Tests for User Story 1 (OPTIONAL - only if tests requested) ⚠️

> **NOTE: Write these tests FIRST, ensure they FAIL before implementation**

- [ ] T010 [P] [US1] Add regression test for [recovery/replication/platform behavior] in tests/[file].cpp
- [ ] T011 [P] [US1] Add integration or restart test for [user journey] in tests/[file].cpp

### Implementation for User Story 1

- [ ] T012 [P] [US1] Update [module] implementation in modules/raft/[module]/[file].cpp
- [ ] T013 [P] [US1] Update supporting logic in modules/raft/[module]/[file].cpp
- [ ] T014 [US1] Integrate the story behavior in modules/raft/[module]/[file].cpp
- [ ] T015 [US1] Add validation, diagnostics, and explicit error reporting in modules/raft/[module]/[file].cpp
- [ ] T016 [US1] Update related contract or config wiring in proto/[file] or modules/raft/common/[file]
- [ ] T017 [US1] Update test entry wiring in tests/CMakeLists.txt, CMakePresets.json, or scripts if needed

**Checkpoint**: At this point, User Story 1 should be fully functional and testable independently

---

## Phase 4: User Story 2 - [Title] (Priority: P2)

**Goal**: [Brief description of what this story delivers]

**Independent Test**: [How to verify this story works on its own]

### Tests for User Story 2 (OPTIONAL - only if tests requested) ⚠️

- [ ] T018 [P] [US2] Add regression test for [durability/catch-up/restart behavior] in tests/[file].cpp
- [ ] T019 [P] [US2] Add integration or multi-node test for [user journey] in tests/[file].cpp

### Implementation for User Story 2

- [ ] T020 [P] [US2] Update [module] logic in modules/raft/[module]/[file].cpp
- [ ] T021 [US2] Update related storage, snapshot, or replication path in modules/raft/[module]/[file].cpp
- [ ] T022 [US2] Integrate story behavior and preserve baseline invariants in modules/raft/[module]/[file].cpp
- [ ] T023 [US2] Update adjacent diagnostics or platform handling in modules/raft/[module]/[file].cpp

**Checkpoint**: At this point, User Stories 1 AND 2 should both work independently

---

## Phase 5: User Story 3 - [Title] (Priority: P3)

**Goal**: [Brief description of what this story delivers]

**Independent Test**: [How to verify this story works on its own]

### Tests for User Story 3 (OPTIONAL - only if tests requested) ⚠️

- [ ] T024 [P] [US3] Add regression test for [observability/platform/script behavior] in tests/[file].cpp
- [ ] T025 [P] [US3] Add end-to-end or recovery test for [user journey] in tests/[file].cpp

### Implementation for User Story 3

- [ ] T026 [P] [US3] Update [module] implementation in modules/raft/[module]/[file].cpp
- [ ] T027 [US3] Add or refine observability, tooling, or cross-platform support in [file path]
- [ ] T028 [US3] Integrate the story with existing validated behavior and document any Linux-specific boundary

**Checkpoint**: All user stories should now be independently functional

---

[Add more user story phases as needed, following the same pattern]

---

## Phase N: Polish & Cross-Cutting Concerns

**Purpose**: Improvements that affect multiple user stories

- [ ] TXXX [P] Documentation updates in docs/ or specs/[###-feature]/
- [ ] TXXX Small-scope cleanup that preserves module boundaries and public behavior
- [ ] TXXX Cross-platform test-entry or script follow-up
- [ ] TXXX [P] Additional regression tests in tests/
- [ ] TXXX Run quickstart.md and CTest validation

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies - can start immediately
- **Foundational (Phase 2)**: Depends on Setup completion - BLOCKS all user stories
- **User Stories (Phase 3+)**: All depend on Foundational phase completion
  - User stories can then proceed in parallel (if staffed)
  - Or sequentially in priority order (P1 → P2 → P3)
- **Polish (Final Phase)**: Depends on all desired user stories being complete

### User Story Dependencies

- **User Story 1 (P1)**: Can start after Foundational (Phase 2) - No dependencies on other stories
- **User Story 2 (P2)**: Can start after Foundational (Phase 2) - May integrate with US1 but should be independently testable
- **User Story 3 (P3)**: Can start after Foundational (Phase 2) - May integrate with US1/US2 but should be independently testable

### Within Each User Story

- Tests for touched high-risk paths MUST be written or updated before finalizing implementation
- Core invariant or contract updates before dependent integration wiring
- Storage, replication, or recovery primitives before higher-level orchestration
- Core implementation before integration
- Story complete before moving to next priority
- Linux-specific validation tasks before closing any Linux-only acceptance criterion

### Parallel Opportunities

- All Setup tasks marked [P] can run in parallel
- All Foundational tasks marked [P] can run in parallel (within Phase 2)
- Once Foundational phase completes, all user stories can start in parallel (if team capacity allows)
- All tests for a user story marked [P] can run in parallel
- Independent module updates within a story marked [P] can run in parallel
- Different user stories can be worked on in parallel by different team members

---

## Parallel Example: User Story 1

```bash
# Launch all tests for User Story 1 together (if tests requested):
Task: "Add regression test for [recovery/replication/platform behavior] in tests/[file].cpp"
Task: "Add integration or restart test for [user journey] in tests/[file].cpp"

# Launch independent module work for User Story 1 together:
Task: "Update [module] implementation in modules/raft/[module]/[file].cpp"
Task: "Update supporting logic in modules/raft/[module]/[file].cpp"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup
2. Complete Phase 2: Foundational (CRITICAL - blocks all stories)
3. Complete Phase 3: User Story 1
4. **STOP and VALIDATE**: Test User Story 1 independently
5. Deploy/demo if ready

### Incremental Delivery

1. Complete Setup + Foundational → Foundation ready
2. Add User Story 1 → Test independently → Deploy/Demo (MVP!)
3. Add User Story 2 → Test independently → Deploy/Demo
4. Add User Story 3 → Test independently → Deploy/Demo
5. Each story adds value without breaking previous stories

### Parallel Team Strategy

With multiple developers:

1. Team completes Setup + Foundational together
2. Once Foundational is done:
   - Developer A: User Story 1
   - Developer B: User Story 2
   - Developer C: User Story 3
3. Stories complete and integrate independently

---

## Notes

- [P] tasks = different files, no dependencies
- [Story] label maps task to specific user story for traceability
- Each user story should be independently completable and testable
- Verify tests fail before implementing
- Commit after each task or logical group
- Stop at any checkpoint to validate story independently
- Avoid: vague tasks, same file conflicts, cross-story dependencies that break independence
- Avoid: rescheduling already stable verified CQUPT_Raft capabilities unless the task is explicitly about industrialization hardening
