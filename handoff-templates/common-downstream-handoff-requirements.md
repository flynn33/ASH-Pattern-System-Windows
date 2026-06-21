# Common Windows Implementation Requirements

## Purpose

This document defines the shared implementation requirements used by the Windows edition of the ASH Pattern System.

## Semantic Boundary

The Windows edition implements the APS semantic contract files under `specs/`. This requirements document constrains repository structure, required deliverables, and proof-of-conformance inputs for the Windows platform. It does not define new semantics.

Specifically:

- `specs/core/` and `specs/algorithms/` define APS behavior.
- `specs/interfaces/contracts/` defines module responsibilities.
- `specs/registries/` defines policy-driven behavior.
- `specs/verification/` defines what must be proven.
- `specs/interfaces/diagnostic-schema.md` and `specs/interfaces/rule-id-taxonomy.md` define diagnostic structure.

## Required Semantic-Module Mapping

The Windows edition must map each of the 9 APS semantic modules to a concrete C++ implementation module:

| APS module | Windows implementation must provide |
|---|---|
| `StateModel` | Full 9-bit state representation, normalization, admissibility, diagnostics, classification, recoverability |
| `RecoveryEngine` | Codeword-based recovery, registry-driven fallback, containment, safe halt, monotonic escalation |
| `RealmEncoder` | Deterministic realm identity encoding from valid 9-bit states |
| `TransitionRegistry` | XOR-by-codeword transition resolution and application |
| `TopologyGenerator` | Deterministic topology generation with stable ordering and lineage |
| `AxiomEvaluator` | Explainable axiom evaluation with diagnostic records |
| `GenerationPlanner` | Abstract plan production with no side effects |
| `ArtifactEmitter` | Plan materialization with no semantic invention |
| `Diagnostics` | Schema-conformant diagnostics with taxonomy-compliant rule IDs |

The mapping is documented in `windows/conformance/module-mapping.md`.

## Required Verification Inputs

The Windows edition must include a verification plan that addresses:

- all invariants defined in `specs/verification/invariant-spec.md`;
- all 5 conformance categories defined in `specs/verification/conformance-categories.md`;
- acceptance criteria defined in `specs/verification/implementation-acceptance.md`;
- Windows build, test, packaging, signing, install, and release-evidence gates.

The active verification plan is `windows/conformance/verification-plan.md`.

## Diagnostics Integration Expectations

The Windows implementation must:

- produce diagnostics conforming to `specs/interfaces/diagnostic-schema.md`;
- use rule IDs conforming to `specs/interfaces/rule-id-taxonomy.md`;
- maintain diagnostic chain integrity from detection through terminal halt;
- never silently omit diagnostics.

## Materialization-Boundary Expectations

The Windows implementation must respect the locked materialization boundary:

- `GenerationPlanner` plans; it does not emit artifacts or perform side effects.
- `ArtifactEmitter` materializes from a plan; it does not invent semantics.
- The plan is the sole interface between planner and emitter.

The active boundary plan is `windows/conformance/materialization-boundary-plan.md`.

## Packaging / Build / Deployment Decisions

The Windows edition must document:

- Visual Studio/MSBuild configuration;
- MSVC and C++ standard settings;
- dependency policy;
- packaging format and distribution decision;
- signing and signature verification;
- supported Windows versions and architectures;
- configuration management.

## Performance / Resource Constraints

The Windows edition must document:

- memory and allocation expectations;
- startup behavior;
- CPU bounds for core operations;
- storage behavior;
- any resource constraints that affect release readiness.

## Caveat / Deviation Tracking

The Windows edition must maintain a deviation log for:

- platform limitations that affect release claims;
- unresolved release blockers;
- Windows-specific decisions that require evidence before release.

Deviations from APS behavior require explicit documentation and must not be silently introduced.

## Proof-Of-Conformance Deliverables

Before the Windows product can move beyond `NOT_SHIPPABLE`, the repository must produce:

1. module mapping document;
2. verification report;
3. diagnostics conformance report;
4. materialization-boundary report;
5. deviation log;
6. acceptance judgment;
7. Windows build and test evidence;
8. packaging, signing, installation, accessibility, security, and release evidence.
