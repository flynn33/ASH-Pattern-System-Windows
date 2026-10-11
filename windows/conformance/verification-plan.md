# Native verification plan

The authoritative inventory is `../tests/test-inventory.json`. Each required executable must exist, launch on a compatible architecture, complete within the bound, report nonempty cases and return success.

The current canonical binding is APS `1.0.0-rc.3` at `50d7e5ab88932a4a967ffc2c83bd18fae45b73c8`; engineering policy remains `0.7.0` at `8dd6e75a06336491b249fac3577db75458db8c13`, and the separate profile catalog is `0.7.2` at `82049af6b10d4dc21d24fb9c67f1b93155e4b7e3`. [Canonical deference](../CANONICAL-DEFERENCE.md) identifies profiles and exact hashes. Preserve original rc.1 and actual rc.2 repair/integration evidence; qualify current resources together. Release acceptance must be established by the exact released `qualification.json`, bound to its clean source and asset bytes.

| Executable | Ownership |
|---|---|
| ash-test-statemodel | Structural state normalization, mapping and classification |
| ash-test-transitionregistry | Exact codewords and all state/codeword XOR pairs |
| ash-test-diagnostics | Envelope construction, identity, ancestry and graph rejection |
| ash-test-recoveryengine | Correction, fallback, containment and terminal guards |
| ash-test-completionmodules | Realm, topology, axiom, planner and emission |
| ash-test-supportdiagnostics | Bounded privacy, profiles, paired export and concurrency |
| ash-test-canonicalconformance | Independently pinned corpus expectations, invariant/category mapping |

The canonical suite uses the validated offline authority and external generated fixtures. The exact invariant headings, rather than an approximate count, define obligations: all 28 IDs and five categories must be mapped to executed checks. Output preserves case IDs and expected/actual fields. Literal conflicts remain failures.

Use the single [qualification entry point](../docs/build-run-instructions.md). Required builds are x64/ARM64 Release and Debug; x64 native execution is required on the assigned host. ARM64 cross-build/link is separate from runtime qualification. Both public SDKs require isolated extracted header and consumer builds, configuration/import failure checks, symbols/dependency inspection and archive integrity.

Independent native-record verification consumes actual output from both x64 configurations, reads the selected immutable transport schemas, recomputes nine-field diagnostic identities and causal chains, and rejects corruption controls. Source-qualified metadata records every generated identity, fixture, library and test-executable digest. A direct solution invocation from the supported product root must succeed with identical completed-output hashes.

Resource validation, static policy checks, builds, executions, peer review, independent acceptance and publication are distinct results. A first complete substantive run may leave only independentReview incomplete. Final independent review binds that actual report's digest/size, all six conclusions with hashed evidence files, the exact clean source fingerprint, separate authority bindings, two Release library hashes and exact two ZIP identities. Optional completed-build reuse verifies all prior substantive gates and current output bytes while rerunning remaining mandatory checks; reused binaries are labeled rather than represented as rebuilt. Missing or stale independent review, failed native core conformance or inconsistent source provenance blocks release.
