# Contract: Failure Injection Boundaries

## Purpose

Define the allowed scope for exact failure injection needed by this
industrialization feature.

## Allowed Injection Surface

- File data sync boundaries for durable writes
- Directory sync boundaries for publish visibility
- Replace/rename boundaries for `meta.bin`, snapshot publish, and log directory
  publish
- Remove/prune boundaries for snapshot cleanup and obsolete segment cleanup
- Partial-write simulation for exact trusted-state and recovery boundary tests

## Forbidden Surface

- Protocol message semantics
- Public RPC contracts
- Persisted file formats, checksums, or record layouts
- Default production behavior when injection is disabled
- Business-layer KV semantics

## Activation Contract

- Injection must be opt-in and test-only.
- Disabled injection must preserve current code paths and return behavior.
- If a helper is introduced, it must remain internal to the implementation
  boundary and not become a public configuration surface.

## Evidence Contract

Each injected failure scenario must record:

- operation name
- affected path or logical publish point
- whether the scenario is Linux-specific
- expected trusted-state outcome
- expected diagnostic signal

## Platform Contract

- Exact crash-style and durability failure injection is Linux-primary in the
  current feature.
- Windows/macOS may initially receive only fallback documentation, build/test
  entrypoints, and deferred runtime validation tasks.
- The absence of runtime-equivalent Windows/macOS injection must be explicit in
  docs and acceptance criteria.

## Acceptance Contract

- Injection-disabled regressions remain green with unchanged semantics.
- Injection-enabled targeted tests fail at the requested boundary and validate
  the expected recovery result.
- No injection mechanism may require rewriting `node`, `replication`, or
  `state_machine` unless storage-local seams are proven insufficient.
