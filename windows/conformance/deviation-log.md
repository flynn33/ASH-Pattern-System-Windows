# Current SDK limits and historical authority corrections

Historical completion records describe earlier work. They do not establish acceptance of SDK `1.0.0-rc.1`.

The current native API exposes supplied operational context and fallback policy instances. No universal domain-safe fallback state is assumed. Applications own candidate verification evidence and runtime context. Persistence and migration-service implementation are outside this SDK scope.

The adopted APS authority is `1.0.0-rc.3` at `50d7e5ab88932a4a967ffc2c83bd18fae45b73c8`. Historical rc.2 at `ea2c6494b0d40bd6975bd210efe33a0cf540812f` repaired the two conflicts observed at original rc.1 commit `7bb68bf3377b18c74d4b0ace6a561371e446bbee`: `APS-VEC-CONTAINMENT-BREACH` expected FAILED despite required SAFE_HALT, and fourteen diagnostic vectors omitted required notes. Original inputs, failing evidence and actual rc.2 repair/integration evidence remain unchanged; current tests bind the complete records to rc.3. These are authoritative successor changes, not local waivers.

Rc.3 preserves rc.2 semantic diagnostic IDs, closes machine-schema gaps for blank explanations and multiline summaries, and explicitly reconciles the named older conflicting clauses without changing the locked mathematics or operational model. Apply only that stated precedence; do not infer blanket acceptance of other stale reference prose or general repository reproducibility. Release acceptance must be established by the exact released `qualification.json`, bound to its clean source and asset bytes.

Engineering policy remains `0.7.0` at `8dd6e75a06336491b249fac3577db75458db8c13`; the adopted realization-profile catalog is separately bound to `0.7.2` at `82049af6b10d4dc21d24fb9c67f1b93155e4b7e3`. The native profile remains `rfd.aps.windows.b1c05f1`, identifying its original baseline. See [canonical deference](../CANONICAL-DEFERENCE.md) for profile hashes and roles. Release acceptance must be established by the exact released `qualification.json`, bound to its clean source and asset bytes.

Typed safety projections describe this engine's entry or adoption time. Imported terminal evidence is locally enforced with `POLICY_HALT_REQUEST`; its unknown external trigger/time is not fabricated. Independent prior histories remain accessible through `retained_diagnostic_chains()` and are exported separately with `Diagnostics::serialize_chain()`. Invalid imported graphs or terminal actions reject before adoption. Public record changes require a clean rebuild/relink.

The intended Windows 10 1809+/Windows 11 floor needs owner physical qualification. ARM64 libraries and consumers must cross-build/link and pass machine inspection; ARM64 runtime is explicitly untested on an x64 host. These can be named noncore candidate limitations.

Distribution is an unsigned Release static SDK ZIP, with `/MT` and C++20/v143 requirements. Authenticode, GUI/installer/UI accessibility and unrelated runtime services are outside this work order. Embedded `/Z7` debug information is inspected; unavoidable Microsoft toolchain path metadata is disclosed in qualification evidence.

See [acceptance judgment](acceptance-judgment.md) for mandatory gates and [compatibility](../docs/sdk-compatibility.md) for the consumer contract.
