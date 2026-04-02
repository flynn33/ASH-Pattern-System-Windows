# AI Coding Handoff

## Purpose

This document tells a coding agent how to use this repository when building a target implementation.

## Handoff rule

Treat this repository as the semantic authority.
Do not infer core semantics from convenience, local idiom, or language defaults.

## Required coding-agent workflow

1. read `README.md`
2. read all files in `docs/`
3. read all files in `specs/core/`, paying particular attention to:
   - `ash-state-space.pseudo.md` — the canonical state definition
   - `control-bit-derivation.pseudo.md` — the control-bit derivation spec and its closure status
   - `core-admissibility.pseudo.md` — the admissibility rules and their closure status
   - `state-validity-diagnostics.pseudo.md` — the canonical diagnostic model
4. read all files in `specs/algorithms/`
5. read `specs/interfaces/semantic-contracts.md`
6. read `governance/repository-governance.md`
7. check for **unresolved closure items** in any spec file — these are foundational design decisions not yet locked
8. only then begin target-specific design and implementation planning

## What the coding agent must preserve

The coding agent must preserve:

- the ASH state space as **F2^9**
- the special role of the first 8 coordinates as the stabilizing algebraic core
- the special role of the 9th coordinate as a derived control/parity dimension
- deterministic normalization
- deterministic realm identity
- deterministic transition behavior
- deterministic topology expansion
- full axiom diagnostics
- explicit separation between generation planning and materialization

## What the coding agent must not do

The coding agent must not:

- reinterpret the 9th coordinate as a normal peer bit for ordinary transitions
- silently collapse the core/control distinction
- make one platform's file structure into the engine's identity
- replace semantic planning with direct side effects
- treat convenience behavior as canonical if the specs do not say so
- invent a control-bit derivation formula if the specification marks it as an unresolved closure item
- invent an admissibility codeword set if the specification marks it as an unresolved closure item
- guess any foundational semantic that is explicitly marked as unresolved in this repository
- silently use a placeholder value for an unresolved closure item in production code

## Unresolved closure items

Some specifications in this repository contain explicitly marked **unresolved closure items**. These are foundational design decisions that have not yet been locked.

The coding agent must:

1. **Identify** all unresolved closure items before beginning implementation.
2. **Report** them as blockers if they affect the target implementation.
3. **Structure code** so that each unresolved item is a single replaceable point of definition (not scattered across the codebase).
4. **Use diagnostic reporting** (e.g., `UNABLE_TO_DERIVE`, `admissibility-law-not-provided`) rather than placeholder values.
5. **Never ship production code** that silently uses a guessed value for an unresolved closure item.

As of the current repository state, the known unresolved closure items are:

- **Derivation formula** — the exact algebraic function `F2^8 -> F2` for `derive_control_bit` (see `specs/core/control-bit-derivation.pseudo.md`)
- **Admissibility law** — the exact codeword set / generator matrix for core admissibility (see `specs/core/core-admissibility.pseudo.md`)

These items must be resolved by explicit design decisions recorded in the specification files before any downstream implementation can be considered complete.

## Required delivery shape for implementation repos

A downstream implementation handoff should include, at minimum:

- mapping from spec modules to implementation modules
- invariant-based test plan
- materialization boundary design
- diagnostics design
- target-runtime constraints
- packaging and build decisions for that target repo
