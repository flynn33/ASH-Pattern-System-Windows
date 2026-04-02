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
│   │   └── realm-identity.pseudo.md
│   ├── algorithms/
│   │   ├── transition-system.pseudo.md
│   │   ├── topology-expansion.pseudo.md
│   │   ├── axiom-evaluation.pseudo.md
│   │   └── generation-planning.pseudo.md
│   └── interfaces/
│       └── semantic-contracts.md
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

### `specs/interfaces/`
Contains contracts that downstream implementations must satisfy.

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
