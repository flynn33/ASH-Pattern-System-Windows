# Source build and SDK verification

The source route uses Visual Studio 2022/MSBuild v143, Windows SDK x64/ARM64 tools, C++20 and PowerShell. Python drives development verification only. Acquire exact pinned resources explicitly outside source; completed-cache qualification is offline.

windows/build/build.ps1 resolves paths from its script directory, supports explicit external outputs, and discovers actual installed tools. Clean cleans only; Rebuild builds. Source changes during compilation invalidate metadata. Native execution requires every executable in the seven-test inventory and rejects missing/empty/failed/timed-out runs.

windows/packaging/verify-sdk-release.ps1 takes an explicit authority cache, original engineering PolicyRoot, separate immutable ProfileRoot/ProfileCommit, issued contracts, separate build/consumer/release/evidence roots and independent review. Selected bindings must match windows/packaging/sdk-authority-bindings.json. It records commands, log hashes, exits, source identities and machine-readable qualification. It never publishes.

The canonical suite maps all 28 actual invariant IDs and five categories. Independent native-record verification validates actual transport records against immutable schemas and recomputes semantic identities and causal chains from both x64 configurations. Required x64 host execution and ARM64 cross-build/link are labeled separately. Archive hashes, isolated headers/consumers, negative settings/imports, replacement/repair/removal and six independent review conclusions with hashed evidence files are mandatory.

Run substantive qualification first without a review. Only a candidate whose sole incomplete mandatory gate is independentReview can support final artifact review. The external review binds the exact clean source fingerprint, separate authorities, prior qualification report hash/size, two Release library hashes and exact two SDK ZIP names/hashes/sizes; it explicitly claims no human approval. Optional CompletedBuildReport reuses only verified completed binaries and preserves their original executed build evidence. Remaining gates run again, including a direct solution invocation from the supported product-root working directory that must leave generated identities, fixtures, library and test output hashes unchanged. See windows/docs/build-run-instructions.md for the command and review-record fields.

Final qualification uses the exact clean integrated commit. Publication adds tag/workflow/download/wiki/synchronization checks. See [Conformance and Release Status](Conformance-and-Release-Status).
