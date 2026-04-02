# Design Philosophy

## Primary principle

The ASH Pattern System must be specified in terms of **semantic invariants**, not implementation accidents.

## Governing ideas

### 1. State before platform

The ASH state model is the root layer.
Platform behavior, tooling, and materialization must be downstream of that state model.

### 2. Semantics before syntax

The repository defines what operations mean.
It does not define the syntax a language must use to express them.

### 3. Planning before side effects

The engine must first produce an abstract, inspectable generation plan.
Only then may a downstream adapter materialize files, modules, services, views, or packages.

### 4. Derived control, not arbitrary mutation

The 9th ASH dimension is a derived control/parity dimension.
It is not treated as a regular peer coordinate under ordinary transitions.

### 5. Stabilization is part of the model

The first 8 coordinates are not just a convenient storage layout.
They are the stabilizing algebraic substrate of the ASH state model.

### 6. Determinism matters

Equal inputs must produce equal semantic outputs for:

- normalization
- control derivation
- realm identity
- transition application
- topology expansion
- axiom diagnosis
- generation planning

### 7. Explanation matters

The engine should expose diagnostics that explain:

- why a state is valid or invalid
- why a transition is allowed or rejected
- why an axiom passes or fails
- why a plan is acceptable or unstable

### 8. Implementations are replaceable

No single language implementation should become the identity of the engine.
The engine must be portable by design.

## Design test

A design decision is aligned only if it preserves the semantic model while leaving room for multiple correct implementations.
