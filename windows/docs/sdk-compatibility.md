# SDK compatibility and qualification

The public product is `ash-core`, producing `ash-core.lib`. SDK `1.0.0-rc.1` uses architecture-specific Release static SDK ZIPs. It adopts APS `1.0.0-rc.3` at `50d7e5ab88932a4a967ffc2c83bd18fae45b73c8`; engineering policy `0.7.0` and realization-profile catalog `0.7.2` have separate [authority bindings](sdk-integration.md#authority-bindings). There is no runtime third-party framework. `/MT` records the consumer's static CRT link contract; it does not prove CRT binary code is already embedded in the archive.

| Surface | Contract / evidence |
|---|---|
| Language | C++20, Microsoft v143 compiler 19.44 or later |
| Linker | Exact produced/consumed Microsoft linker recorded in release qualification; earlier linkers unverified |
| Runtime | Release `/MT`, release iterator debugging level zero |
| Symbols | `/Z7` object debug information, no `/GL`; actual archive and consumer symbol inspection required |
| Architectures | x64 and ARM64 builds and extracted consumer links required |
| x64 execution | Required on the assigned compatible Windows host |
| ARM64 execution | Only when available; cross-built binaries are not runtime-qualified |
| Deployment floor | Intended Windows 10 1809+ and Windows 11; untested OS/hardware combinations remain owner-phase qualification |
| Distribution | Unsigned ZIP prerelease; checksums are not signatures |

The actual tested matrix is in the versioned qualification report. A build, JSON parse, static policy report, or historical pass total alone is insufficient. The complete native canonical threshold, support-diagnostics tests, negative consumer/import/ZIP checks and independent delivery review are mandatory before publication.

During this prerelease, the public diagnostic root field changes from `shared_ptr` to `weak_ptr`, diagnostic layouts gain structured details/occurrence metadata, and typed containment/halt projections retain their full ancestry and local entry/adoption times. There is no ABI compatibility claim with old unverified binaries. Clean rebuild/relink every consumer. See [sdk-migration.md](sdk-migration.md). Mutable semantic objects and their records remain caller-serialized; support-session concurrency does not extend that guarantee.

Generation plan and emitted artifact layouts now retain complete request, topology, role, axiom and descriptor evidence. The legacy planner overload remains available with explicit source defaults, while `aps-windows-plan-2` replaces the earlier incomplete identity payload and changes plan hashes. Regenerate stored plans rather than relabeling old hashes; [planning integration](sdk-integration.md#planning-and-descriptor-emission) documents the supported native references.

The corrected immutable rc.3 authority resolves the earlier rc.1 diagnostic-notes and containment-breach conflicts; original evidence stays historical. Release acceptance must be established by the exact released `qualification.json`, bound to its clean source and asset bytes. The product does not claim literal corpus conformance by comparing only convenient subsets or treating a failed mandatory vector as a caveat. ARM64 cross-build/link evidence does not establish runtime execution. Owner physical qualification, deployment-floor checks and unavailable ARM64 runtime coverage remain explicit owner phases; a core failure cannot become one of those limitations.
