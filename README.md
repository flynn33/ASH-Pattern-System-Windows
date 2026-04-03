# ASH Pattern System

## Canonical Agnostic Repository

This repository is the **platform-neutral, language-neutral source of truth** for the ASH Pattern System.

It is the agnostic specification repository built around the corrected ASH state-space model and the current ASH design philosophy.

## Repository purpose

The ASH Pattern System is a **platform-agnostic and code-agnostic framework** for:

- **self-healing software** — systems that detect and correct their own state errors
- **self-correcting software** — systems that restore valid state from corrupted state
- **safe-failure behavior** — systems that halt safely when correction is impossible
- **fallback behavior** — systems that select known-good states when correction is ambiguous
- **deterministic recovery planning** — systems that plan recovery actions from structured diagnostics
- **resilient system design** — systems that degrade gracefully through containment rather than fail silently

This repository defines:

- the canonical ASH state space in **F2^9**
- the stabilizing algebraic role of the first 8 coordinates
- the derived role of the 9th control/parity dimension
- system-state classification (stable, unstable, correctable, degraded, contained, failed, safe-halt)
- recoverability semantics and deterministic recovery-category mapping
- recovery, fallback, containment, and safe-failure algorithms
- canonical fallback-policy registry for deterministic fallback selection
- unified diagnostic schema and rule-ID taxonomy for auditable diagnostics
- locked implementation contracts for all 9 required semantic modules
- invariant-based verification requirements for downstream conformance
- deterministic transition semantics
- deterministic topology expansion semantics
- axiom evaluation semantics
- generation-planning semantics
- implementation contracts for future coding agents

This repository does **not** define:

- a canonical programming language
- a canonical operating system or platform
- a canonical build system
- a canonical CLI
- a canonical UI or rendering layer
- a canonical filesystem layout for emitted projects

## Design stance

The engine core must remain stable even when downstream implementations differ.

That means:

- the **specifications are canonical**
- implementations are adapters to the specifications
- semantics come before syntax
- planning comes before materialization
- the state model comes before platform behavior

## Core state-space correction

The ASH state space is defined as **F2^9**.

Within that space:

- coordinates `b0` through `b7` form the primary stabilizing algebraic core
- that 8-bit core is modeled against the **[8,4,4] extended Hamming-code structure**
- coordinate `b8` is a **derived control/parity dimension** used for stabilization, detection, and self-referential state semantics

The 9th coordinate is therefore **not** treated as an ordinary peer bit for unrestricted direct mutation during ordinary state evolution.

## Repository map

- `docs/00-repository-purpose.md` — what this repository is for
- `docs/01-design-philosophy.md` — governing design principles
- `docs/02-target-repository-shape.md` — canonical repository structure
- `docs/03-design-roadmap.md` — planning sequence for future work
- `specs/core/ash-state-space.pseudo.md` — canonical ASH state definition
- `specs/core/control-bit-derivation.pseudo.md` — control-bit derivation semantics and closure status
- `specs/core/core-admissibility.pseudo.md` — core admissibility rules and state classification
- `specs/core/state-validity-diagnostics.pseudo.md` — canonical state-validity diagnostic model
- `specs/core/system-state-classification.pseudo.md` — canonical system-state classes and class-to-action mapping
- `specs/core/recoverability-semantics.pseudo.md` — recoverability categories and deterministic recovery mapping
- `specs/core/realm-identity.pseudo.md` — realm identity and encoding semantics
- `specs/algorithms/recovery-fallback-semantics.pseudo.md` — deterministic recovery and fallback selection
- `specs/algorithms/containment-safe-failure-semantics.pseudo.md` — containment and safe-failure behavior
- `specs/algorithms/transition-system.pseudo.md` — transition semantics
- `specs/algorithms/topology-expansion.pseudo.md` — topology generation semantics
- `specs/algorithms/axiom-evaluation.pseudo.md` — axiom evaluation semantics
- `specs/algorithms/generation-planning.pseudo.md` — abstract generation planning flow
- `specs/registries/fallback-policy-registry.md` — canonical fallback-policy registry for deterministic fallback selection
- `specs/interfaces/semantic-contracts.md` — umbrella contract document referencing detailed module contracts
- `specs/interfaces/diagnostic-schema.md` — shared diagnostic envelope for all diagnostic contexts
- `specs/interfaces/rule-id-taxonomy.md` — canonical rule-ID structure and governance
- `specs/interfaces/contracts/state-model-contract.md` — StateModel implementation contract
- `specs/interfaces/contracts/recovery-engine-contract.md` — RecoveryEngine implementation contract
- `specs/interfaces/contracts/realm-encoder-contract.md` — RealmEncoder implementation contract
- `specs/interfaces/contracts/transition-registry-contract.md` — TransitionRegistry implementation contract
- `specs/interfaces/contracts/topology-generator-contract.md` — TopologyGenerator implementation contract
- `specs/interfaces/contracts/axiom-evaluator-contract.md` — AxiomEvaluator implementation contract
- `specs/interfaces/contracts/generation-planner-contract.md` — GenerationPlanner implementation contract
- `specs/interfaces/contracts/artifact-emitter-contract.md` — ArtifactEmitter implementation contract
- `specs/interfaces/contracts/diagnostics-module-contract.md` — Diagnostics module implementation contract
- `specs/verification/invariant-spec.md` — canonical invariant set for downstream conformance verification
- `specs/verification/conformance-categories.md` — 5 conformance categories for verification grouping
- `specs/verification/implementation-acceptance.md` — acceptance criteria and judgment language
- `governance/repository-governance.md` — governance rules for this repository
- `governance/ai-coding-handoff.md` — handoff instructions for coding agents

## Intended use

A future coding agent should receive this repository plus a build-target instruction such as:

- implement the ASH Pattern System for Rust
- implement the ASH Pattern System for Swift
- implement the ASH Pattern System for C++
- implement the ASH Pattern System for TypeScript
- implement the ASH Pattern System for a service runtime
- implement the ASH Pattern System for a mobile runtime

The build target may change.
The semantic source of truth must not.
