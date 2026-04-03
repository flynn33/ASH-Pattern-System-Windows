# Target Repository Shape

## Canonical structure

```text
ash-pattern-system/
├── README.md
├── docs/
│   ├── 00-repository-purpose.md
│   ├── 01-design-philosophy.md
│   ├── 02-target-repository-shape.md
│   └── 03-design-roadmap.md
├── specs/
│   ├── core/
│   │   ├── ash-state-space.pseudo.md
│   │   ├── control-bit-derivation.pseudo.md
│   │   ├── core-admissibility.pseudo.md
│   │   ├── state-validity-diagnostics.pseudo.md
│   │   ├── system-state-classification.pseudo.md
│   │   ├── recoverability-semantics.pseudo.md
│   │   └── realm-identity.pseudo.md
│   ├── algorithms/
│   │   ├── codeword-transformation-semantics.pseudo.md
│   │   ├── averaging-operator-semantics.pseudo.md
│   │   ├── branching-semantics.pseudo.md
│   │   ├── transition-system.pseudo.md
│   │   ├── topology-expansion.pseudo.md
│   │   ├── axiom-evaluation.pseudo.md
│   │   ├── generation-planning.pseudo.md
│   │   ├── recovery-fallback-semantics.pseudo.md
│   │   └── containment-safe-failure-semantics.pseudo.md
│   ├── registries/
│   │   └── fallback-policy-registry.md
│   ├── verification/
│   │   ├── invariant-spec.md
│   │   ├── conformance-categories.md
│   │   └── implementation-acceptance.md
│   └── interfaces/
│       ├── semantic-contracts.md
│       ├── diagnostic-schema.md
│       ├── rule-id-taxonomy.md
│       └── contracts/
│           ├── state-model-contract.md
│           ├── recovery-engine-contract.md
│           ├── realm-encoder-contract.md
│           ├── transition-registry-contract.md
│           ├── topology-generator-contract.md
│           ├── axiom-evaluator-contract.md
│           ├── generation-planner-contract.md
│           ├── artifact-emitter-contract.md
│           └── diagnostics-module-contract.md
└── governance/
    ├── repository-governance.md
    └── ai-coding-handoff.md
```

## Structural rules

### `docs/`
Contains explanatory and planning documents.
These explain intent, philosophy, and sequencing.

### `specs/core/`
Contains the canonical model of the ASH state itself.
This is the highest-priority semantic layer.

### `specs/algorithms/`
Contains algorithmic semantics expressed in platform-neutral pseudocode and prose.

### `specs/registries/`
Contains canonical registries that govern deterministic policy-driven behavior (e.g., fallback selection).

### `specs/interfaces/`
Contains contracts, diagnostic schemas, and rule taxonomies that downstream implementations must satisfy.

### `governance/`
Contains repository rules and handoff rules for coding agents.

## Exclusions from this repository shape

This repository shape intentionally excludes canonical dependence on:

- `src/`
- `include/`
- `tests/`
- `examples/`
- build-system files
- runtime package manifests
- platform-specific CI assumptions

Those may exist later in implementation repositories.
They are not the identity of this repository.
