# Repository Governance

## Repository role

This repository owns the Windows native realization of the ASH Pattern System:
C++20 semantic modules, their MSBuild projects and tests, and the static
`ash-core.lib` SDK. Native source, build files, packaging metadata and integration
material belong here. The SDK consumer boundary is the curated release payload,
not a wholesale import of this repository.

Canonical semantics are defined by the immutable external
[APS 1.0.0-rc.3 source](https://github.com/flynn33/ASH-Pattern-System/tree/50d7e5ab88932a4a967ffc2c83bd18fae45b73c8).
Copied specifications and historical handoffs in this repository do not override
that authority. [Canonical deference](../windows/CANONICAL-DEFERENCE.md) records
the owning precedence decisions and native realization boundary.

## Authority and policy bindings

The [machine-readable bindings](../windows/packaging/sdk-authority-bindings.json)
keep three roles distinct:

| Role | Version | Immutable source commit |
|---|---|---|
| Canonical semantic authority | `1.0.0-rc.3` | `50d7e5ab88932a4a967ffc2c83bd18fae45b73c8` |
| Engineering policy | `0.7.0` | `8dd6e75a06336491b249fac3577db75458db8c13` |
| Realization-profile catalog | `0.7.2` | `82049af6b10d4dc21d24fb9c67f1b93155e4b7e3` |

Updating a catalog does not upgrade engineering policy or redefine mathematics.
The native profile `rfd.aps.windows.b1c05f1` names the original native baseline;
the SDK product version and current source commit remain separate identities.
Upstream [canonical repository governance](https://github.com/flynn33/ASH-Pattern-System/blob/50d7e5ab88932a4a967ffc2c83bd18fae45b73c8/governance/repository-governance.md)
is an external reference for its specification repository, not a ban on this
product's native source or build/package files.

## Semantic and safety precedence

Windows, C++ and compiler choices describe this realization, not the canonical
identity of APS. Native implementation changes must preserve the pinned
nine-dimensional binary state model, canonical codewords, realm/orbit transforms,
averaging, topology and axiom predicates. Convenience behavior, copied prose and
generated self-agreement cannot replace the owning canonical contracts.

Structural normalization and operational assessment have separate responsibilities.
Fallback requires the supplied policy and verification evidence. Prevented
contained operations retain blocked evidence; a confirmed containment breach
requires terminal SAFE_HALT. Terminal records and their causal history remain
frozen. Required diagnostic identity, explanations, structured facts and safety
transports are part of conformance, not optional logging.

Resolve a semantic ambiguity through an explicit owning decision and immutable
authority binding before treating it as settled in production. Native fixes must
not invent locked algebraic values, substitute placeholders, weaken mandatory
checks or claim that administrator bypass waives correctness. Preserve historical
failure evidence when adopting a correction.

## Source and consumer boundaries

Files admitted here must support the Windows implementation, its tests/build,
SDK packaging, consumer integration, documentation or repository maintenance.
Upstream research remains external to the consumer dependency graph. SDK consumers
use the curated headers, Release library, integration files and authored sample;
implementation source, test binaries, governance and authority repositories are
not SDK dependencies. See [SDK integration](../windows/docs/sdk-integration.md).

The owner-maintained contribution and security policies remain in
[CONTRIBUTING.md](../CONTRIBUTING.md) and [SECURITY.md](../SECURITY.md).
Outside contributions are not accepted. Repository access controls and rulesets
restrict writes to the owner; automated checks do not grant write authority or
constitute human approval.

## GitHub checks and release evidence

The actual local workflow is the [attribution sentinel](../.github/workflows/no-ai-attribution.yml),
implemented by [no_attribution_check.py](../.github/scripts/no_attribution_check.py).
Its responsibilities and event boundaries are documented in
[GitHub governance agents](github-agents-governance.md). It checks attribution
metadata and content; it does not execute mathematics, native conformance, builds
or SDK consumer acceptance.

Native and artifact qualification use the supported
[release verification entry point](../windows/packaging/verify-sdk-release.ps1)
and the [required acceptance evidence](../windows/conformance/acceptance-judgment.md).
All mandatory semantic, native, diagnostic, consumer and integrity gates must
pass. Release acceptance is established by the exact released `qualification.json`
bound to its clean source and asset bytes; historical totals, a passing sentinel
or a scoped prerequisite review cannot establish that result.
